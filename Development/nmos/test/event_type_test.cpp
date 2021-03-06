// The first "test" is of course whether the header compiles standalone
#include "nmos/event_type.h"

#include "bst/test/test.h"

////////////////////////////////////////////////////////////////////////////////////////////
BST_TEST_CASE(testIsMatchingEventType)
{
    const auto boolean = nmos::event_types::boolean;
    const auto number = nmos::event_types::number;
    const auto number_wildcard = nmos::event_types::measurement(nmos::event_types::number, nmos::event_types::wildcard);
    const auto temperature = nmos::event_types::measurement(nmos::event_types::number, U("temperature")); // not sure that IS-07 allows this form?
    const auto temperature_Celsius = nmos::event_types::measurement(nmos::event_types::number, U("temperature"), U("C"));
    const auto temperature_Fahrenheit = nmos::event_types::measurement(nmos::event_types::number, U("temperature"), U("F"));
    const auto temperature_wildcard = nmos::event_types::measurement(nmos::event_types::number, U("temperature"), nmos::event_types::wildcard);

    BST_REQUIRE(nmos::is_matching_event_type(boolean, boolean));
    BST_REQUIRE(!nmos::is_matching_event_type(boolean, number));
    BST_REQUIRE(!nmos::is_matching_event_type(number, boolean));
    BST_REQUIRE(nmos::is_matching_event_type(number, number));

    BST_REQUIRE(!nmos::is_matching_event_type(number, temperature));

    BST_REQUIRE(nmos::is_matching_event_type(temperature, temperature));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature, number));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature, temperature_Celsius));

    BST_REQUIRE(nmos::is_matching_event_type(temperature_Celsius, temperature_Celsius));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature_Celsius, temperature_Fahrenheit));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature_Fahrenheit, temperature_Celsius));
    BST_REQUIRE(nmos::is_matching_event_type(temperature_Fahrenheit, temperature_Fahrenheit));

    BST_REQUIRE(nmos::is_matching_event_type(temperature_wildcard, temperature_Celsius));
    BST_REQUIRE(nmos::is_matching_event_type(temperature_wildcard, temperature_Fahrenheit));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature_wildcard, boolean));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature_wildcard, number));
    BST_REQUIRE(!nmos::is_matching_event_type(temperature_wildcard, temperature));
}
