#ifndef NMOS_NODE_RESOURCES_H
#define NMOS_NODE_RESOURCES_H

#include "nmos/id.h"
#include "nmos/rational.h"
#include "nmos/settings.h"

namespace web
{
    class uri;
}

namespace nmos
{
    struct channel;
    struct colorspace;
    enum chroma_subsampling : int;
    struct format;
    struct interlace_mode;
    struct resource;
    struct transfer_characteristic;
    struct transport;

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/device.json
    nmos::resource make_device(const nmos::id& id, const nmos::id& node_id, const std::vector<nmos::id>& senders, const std::vector<nmos::id>& receivers, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_core.json
    nmos::resource make_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_generic.json
    nmos::resource make_generic_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::format& format, const nmos::settings& settings);
    nmos::resource make_video_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);
    nmos::resource make_data_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

	// See https://amwa-tv.github.io/nmos-event-tally/tags/v1.0/docs/4.0._Core_models.html#21-sources
	nmos::resource make_event_source(const nmos::id& id, const nmos::id& device_id, const utility::string_t& event_type, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-event-tally/blob/v1.1-dev/docs/6.0.%20Event%20and%20tally%20rest%20api.md
    nmos::resource make_restapi_event(const nmos::id& source_id, const web::json::value& type, const web::json::value& state);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/source_audio.json
    nmos::resource make_audio_source(const nmos::id& id, const nmos::id& device_id, const nmos::rational& grain_rate, const std::vector<nmos::channel>& channels, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_core.json
    nmos::resource make_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, const nmos::settings& settings);

	// See https://amwa-tv.github.io/nmos-event-tally/tags/v1.0/docs/4.0._Core_models.html#22-flows
    nmos::resource make_event_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_video.json
    nmos::resource make_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_video_raw.json
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& grain_rate, unsigned int frame_width, unsigned int frame_height, const nmos::interlace_mode& interlace_mode, const nmos::colorspace& colorspace, const nmos::transfer_characteristic& transfer_characteristic, chroma_subsampling chroma_subsampling, unsigned int bit_depth, const nmos::settings& settings);
    nmos::resource make_raw_video_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_audio.json
    nmos::resource make_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/APIs/schemas/flow_audio_raw.json
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::rational& sample_rate, unsigned int bit_depth, const nmos::settings& settings);
    nmos::resource make_raw_audio_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/flow_sdianc_data.json
    nmos::resource make_sdianc_data_flow(const nmos::id& id, const nmos::id& source_id, const nmos::id& device_id, const nmos::settings& settings);

    web::uri make_connection_api_transportfile(const nmos::id& sender_id, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/sender.json
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::transport& transport, const nmos::id& device_id, const utility::string_t& manifest_href, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);
    nmos::resource make_sender(const nmos::id& id, const nmos::id& flow_id, const nmos::id& device_id, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_core.json
    nmos::resource make_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_video.json
    nmos::resource make_video_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_audio.json
    nmos::resource make_audio_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, unsigned int bit_depth, const nmos::settings& settings);

    // See https://github.com/AMWA-TV/nmos-discovery-registration/blob/v1.2/APIs/schemas/receiver_data.json
    nmos::resource make_sdianc_data_receiver(const nmos::id& id, const nmos::id& device_id, const nmos::transport& transport, const std::vector<utility::string_t>& interfaces, const nmos::settings& settings);

    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7);
    nmos::resource make_connection_websocket_sender(const nmos::id& id, const nmos::id& device_id, const nmos::id& source_id, const nmos::settings& settings);
    nmos::resource make_connection_mqtt_sender(const nmos::id& sender_id, const utility::string_t& destination_host, const utility::string_t& destination_port, const nmos::id& source_id, const nmos::settings& settings);

    web::json::value make_connection_sender_transportfile(const utility::string_t& transportfile);
    web::json::value make_connection_sender_transportfile(const web::uri& transportfile);
    // transportfile may be URL or the contents of the SDP file (yeah, yuck)
    nmos::resource make_connection_sender(const nmos::id& id, bool smpte2022_7, const utility::string_t& transportfile);

    nmos::resource make_connection_receiver(const nmos::id& id, bool smpte2022_7);
}

#include "nmos/resources.h"

// This currently serves as an example of the resources that an NMOS node would construct
namespace nmos
{
    struct node_model;

    namespace experimental
    {
        // insert a node resource, and sub-resources, according to the settings; return an iterator to the inserted node resource,
        // or to a resource that prevented the insertion, and a bool denoting whether the insertion took place
        std::pair<resources::iterator, bool> insert_node_resources(nmos::resources& node_resources, const nmos::settings& settings);
    }
}

#endif
