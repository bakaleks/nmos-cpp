#include <fstream>
#include <iostream>
#include "cpprest/host_utils.h"
#include "nmos/admin_ui.h"
#include "nmos/api_utils.h"
#include "nmos/connection_api.h"
#include "nmos/events_api.h"
#include "nmos/events_ws_api.h"
#include "nmos/logging_api.h"
#include "nmos/model.h"
#include "nmos/node_api.h"
#include "nmos/node_behaviour.h"
#include "nmos/node_resources.h"
#include "nmos/process_utils.h"
#include "nmos/server_utils.h"
#include "nmos/settings_api.h"
#include "nmos/slog.h"
#include "nmos/thread_utils.h"
#include "main_gate.h"
#include "node_implementation.h"

int main(int argc, char* argv[])
{
    // Construct our data models including mutexes to protect them

    nmos::node_model node_model;

    nmos::experimental::log_model log_model;
    std::atomic<slog::severity> level{ slog::severities::more_info };

    // Streams for logging, initially configured to write errors to stderr and to discard the access log
    std::filebuf error_log_buf;
    std::ostream error_log(std::cerr.rdbuf());
    std::filebuf access_log_buf;
    std::ostream access_log(&access_log_buf);

    // Logging should all go through this logging gateway
    main_gate gate(error_log, access_log, log_model, level);

    try
    {
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Starting nmos-cpp node";

        // Settings can be passed on the command-line, directly or in a configuration file, and a few may be changed dynamically by PATCH to /settings/all on the Settings API
        //
        // * "logging_level": integer value, between 40 (least verbose, only fatal messages) and -40 (most verbose)
        // * "registry_address": used to construct request URLs for registry APIs (if not discovered via DNS-SD)
        //
        // E.g.
        //
        // # ./nmos-cpp-node "{\"logging_level\":-40}"
        // # ./nmos-cpp-node config.json
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -d "{\"logging_level\":-40}"
        // # curl -X PATCH -H "Content-Type: application/json" http://localhost:3209/settings/all -T config.json

        if (argc > 1)
        {
            std::error_code error;
            node_model.settings = web::json::value::parse(utility::s2us(argv[1]), error);
            if (error)
            {
                std::ifstream file(argv[1]);
                node_model.settings = web::json::value::parse(file, error);
            }
            if (error || !node_model.settings.is_object())
            {
                slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Bad command-line settings [" << error << "]";
                return -1;
            }
        }

        // Prepare run-time default settings (different than header defaults)

        web::json::insert(node_model.settings, std::make_pair(nmos::experimental::fields::seed_id, web::json::value::string(nmos::make_id())));

        web::json::insert(node_model.settings, std::make_pair(nmos::fields::logging_level, web::json::value::number(level)));
        level = nmos::fields::logging_level(node_model.settings); // synchronize atomic value with settings

        // if the "host_addresses" setting was omitted, add all the interface addresses
        const auto interface_addresses = web::http::experimental::interface_addresses();
        if (!interface_addresses.empty())
        {
            web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_addresses, web::json::value_from_elements(interface_addresses)));
        }

        // if the "host_address" setting was omitted, use the first of the "host_addresses"
        if (node_model.settings.has_field(nmos::fields::host_addresses))
        {
            web::json::insert(node_model.settings, std::make_pair(nmos::fields::host_address, nmos::fields::host_addresses(node_model.settings)[0]));
        }

        // if any of the specific "<api>_port" settings were omitted, use "http_port" if present
        if (node_model.settings.has_field(nmos::fields::http_port))
        {
            const auto http_port = nmos::fields::http_port(node_model.settings);
            web::json::insert(node_model.settings, std::make_pair(nmos::fields::registration_port, http_port));
            web::json::insert(node_model.settings, std::make_pair(nmos::fields::node_port, http_port));
            web::json::insert(node_model.settings, std::make_pair(nmos::fields::connection_port, http_port));
            web::json::insert(node_model.settings, std::make_pair(nmos::experimental::fields::settings_port, http_port));
            web::json::insert(node_model.settings, std::make_pair(nmos::experimental::fields::logging_port, http_port));
        }

        // Reconfigure the logging streams according to settings
        // (obviously, until this point, the logging gateway has its default behaviour...)

        if (!nmos::fields::error_log(node_model.settings).empty())
        {
            error_log_buf.open(nmos::fields::error_log(node_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            error_log.rdbuf(&error_log_buf);
        }

        if (!nmos::fields::access_log(node_model.settings).empty())
        {
            access_log_buf.open(nmos::fields::access_log(node_model.settings), std::ios_base::out | std::ios_base::app);
            auto lock = log_model.write_lock();
            access_log.rdbuf(&access_log_buf);
        }

        // Log the process ID and the API addresses we'll be using

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Process ID: " << nmos::details::get_process_id();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Initial settings: " << node_model.settings.serialize();
        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Configuring nmos-cpp node with its primary Node API at: " << nmos::fields::host_address(node_model.settings) << ":" << nmos::fields::node_port(node_model.settings);

        // Set up the APIs, assigning them to the configured ports

        typedef std::pair<utility::string_t, int> address_port;
        std::map<address_port, web::http::experimental::listener::api_router> port_routers;

        // Configure the Settings API

        const address_port settings_address(nmos::experimental::fields::settings_address(node_model.settings), nmos::experimental::fields::settings_port(node_model.settings));
        port_routers[settings_address].mount({}, nmos::experimental::make_settings_api(node_model, level, gate));

        // Configure the Logging API

        const address_port logging_address(nmos::experimental::fields::logging_address(node_model.settings), nmos::experimental::fields::logging_port(node_model.settings));
        port_routers[logging_address].mount({}, nmos::experimental::make_logging_api(log_model, gate));

        // Configure the Node API

        nmos::node_api_target_handler target_handler = nmos::make_node_api_target_handler(node_model);
        port_routers[{ {}, nmos::fields::node_port(node_model.settings) }].mount({}, nmos::make_node_api(node_model, target_handler, gate));

        // start the underlying implementation and set up the node resources
        auto node_resources = nmos::details::make_thread_guard([&] { node_implementation_thread(node_model, gate); }, [&] { node_model.controlled_shutdown(); });

        // Configure the Connection API

        port_routers[{ {}, nmos::fields::connection_port(node_model.settings) }].mount({}, nmos::make_connection_api(node_model, gate));

        // Configure the IS-07 Events API
        port_routers[{ {}, nmos::fields::events_port(node_model.settings) }].mount({}, nmos::make_events_api(node_model, gate));

        nmos::websockets node_websockets;

        web::websockets::experimental::listener::validate_handler eventntally_ws_validate_handler = nmos::make_eventntally_ws_validate_handler(node_model, gate);
        web::websockets::experimental::listener::open_handler eventntally_ws_open_handler = nmos::make_eventntally_ws_open_handler(node_model, node_websockets, gate);
        web::websockets::experimental::listener::close_handler eventntally_ws_close_handler = nmos::make_eventntally_ws_close_handler(node_model, node_websockets, gate);
        web::websockets::experimental::listener::message_handler eventntally_ws_message_handler = nmos::make_eventntally_ws_message_handler(node_model, node_websockets, gate);
        web::websockets::experimental::listener::websocket_listener eventntally_ws_listener(nmos::fields::eventntally_ws_port(node_model.settings), nmos::make_slog_logging_callback(gate));
        eventntally_ws_listener.set_validate_handler(std::ref(eventntally_ws_validate_handler));
        eventntally_ws_listener.set_open_handler(std::ref(eventntally_ws_open_handler));
        eventntally_ws_listener.set_close_handler(std::ref(eventntally_ws_close_handler));
        eventntally_ws_listener.set_message_handler(std::ref(eventntally_ws_message_handler));

        // Set up the listeners for each API port

        // try to use the configured TCP listen backlog
        web::http::experimental::listener::http_listener_config listener_config;
        listener_config.set_backlog(nmos::fields::listen_backlog(node_model.settings));

        std::vector<web::http::experimental::listener::http_listener> port_listeners;
        for (auto& port_router : port_routers)
        {
            // default empty string means the wildcard address
            const auto& router_address = !port_router.first.first.empty() ? port_router.first.first : web::http::experimental::listener::host_wildcard;
            // map the configured client port to the server port on which to listen
            // hmm, this should probably also take account of the address
            port_listeners.push_back(nmos::make_api_listener(router_address, nmos::experimental::server_port(port_router.first.second, node_model.settings), port_router.second, listener_config, gate));
        }

        // Open the API ports

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Preparing for connections";

        std::vector<web::http::experimental::listener::http_listener_guard> port_guards;

        for (auto& port_listener : port_listeners)
        {
            if (0 <= port_listener.uri().port()) port_guards.push_back({ port_listener });
        }

        web::websockets::experimental::listener::websocket_listener_guard eventntally_ws_guard;
        if (0 <= eventntally_ws_listener.port()) eventntally_ws_guard = { eventntally_ws_listener };
        auto send_eventntally_ws_events = nmos::details::make_thread_guard([&] { nmos::send_eventntally_ws_events_thread(eventntally_ws_listener, node_model, node_websockets, gate); }, [&] { node_model.controlled_shutdown(); });
        auto update_temperature_state = nmos::details::make_thread_guard([&] { node_update_temperature_thread(node_model, gate); }, [&] { node_model.controlled_shutdown(); });
        auto erase_expired_resources = nmos::details::make_thread_guard([&] { nmos::erase_expired_resources_thread(node_model, gate); }, [&] { node_model.controlled_shutdown(); });
        

        // Start up node operation (including the mDNS advertisements) once all NMOS APIs are open

        auto node_behaviour = nmos::details::make_thread_guard([&] { nmos::node_behaviour_thread(node_model, gate); }, [&] { node_model.controlled_shutdown(); });

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Ready for connections";

        // Wait for a process termination signal
        nmos::details::wait_term_signal();

        slog::log<slog::severities::info>(gate, SLOG_FLF) << "Closing connections";
    }
    catch (const web::json::json_exception& e)
    {
        // most likely from incorrect types in the command line settings
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "JSON error: " << e.what();
    }
    catch (const web::http::http_exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "HTTP error: " << e.what() << " [" << e.error_code() << "]";
    }
    catch (const std::system_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "System error: " << e.what() << " [" << e.code() << "]";
    }
    catch (const std::runtime_error& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Implementation error: " << e.what();
    }
    catch (const std::exception& e)
    {
        slog::log<slog::severities::error>(gate, SLOG_FLF) << "Unexpected exception: " << e.what();
    }
    catch (...)
    {
        slog::log<slog::severities::severe>(gate, SLOG_FLF) << "Unexpected unknown exception";
    }

    slog::log<slog::severities::info>(gate, SLOG_FLF) << "Stopping nmos-cpp node";

    return 0;
}
