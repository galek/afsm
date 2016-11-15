/*
 * vending_machine_test.cpp
 *
 *  Created on: Nov 15, 2016
 *      Author: zmij
 */

#include <gtest/gtest.h>
#include "vending_machine.hpp"

namespace vending {

using result = ::afsm::actions::event_process_result;

TEST(Vending, OnOff)
{
    vending_machine vm;

    EXPECT_EQ(0ul, vm.count()) << "No goods were loaded by default constructor";
    EXPECT_TRUE(vm.is_empty()) << "Vending machine is empty";

    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_in_state< vending_def::on >())
                                << "Vending machine is not on";

    EXPECT_TRUE(done(vm.process_event(events::power_on{})))
                                << "Vending machine turns on correctly";

    EXPECT_FALSE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is not off";
    EXPECT_TRUE(vm.is_in_state< vending_def::on >())
                                << "Vending machine is on";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::out_of_service >())
                                << "Vending machine is out of service (empty)";

    EXPECT_TRUE(done(vm.process_event(events::power_off{})))
                                << "Vending machine turns off correctly";

    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_in_state< vending_def::on >())
                                << "Vending machine is not on";
}

TEST(Vending, Maintenance)
{
    vending_machine vm;

    // Try turn to maintenance when off
    EXPECT_FALSE(done(vm.process_event(events::start_maintenance{100500})))
                                << "Vending machine allows maintenance only when on";
    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine stays off";

    // Power on
    EXPECT_TRUE(done(vm.process_event(events::power_on{})))
                                << "Vending machine turns on correctly";

    // Try use an invalid code
    EXPECT_FALSE(ok(vm.process_event(events::start_maintenance{100500})))
                                << "Vending machine rejects incorrect code";
    EXPECT_TRUE(vm.is_in_state< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::out_of_service >())
                                << "Vending machine stays in previous state";

    // Use a correct code
    EXPECT_TRUE(done(vm.process_event(events::start_maintenance{vending_machine::factory_code})))
                                << "Vending machine accepts factory code";
    EXPECT_TRUE(vm.is_in_state< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    // Exit maintenance
    EXPECT_TRUE(done(vm.process_event(events::end_maintenance{})))
                                << "Vending machine exits maintenance";
    EXPECT_TRUE(vm.is_in_state< vending_def::on >())
                                << "Vending machine stays on";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::out_of_service >())
                                << "Vending machine transits to out of service (empty)";

    // Use a correct code
    EXPECT_TRUE(done(vm.process_event(events::start_maintenance{vending_machine::factory_code})))
                                << "Vending machine accepts factory code";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    // Turn off while in maintenance mode
    EXPECT_TRUE(done(vm.process_event(events::power_off{})))
                                << "Vending machine turns off correctly";

    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is off";

    // Turn back on
    EXPECT_TRUE(done(vm.process_event(events::power_on{})))
                                << "Vending machine turns on correctly";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance >())
                                << "Vending machine transits to maintenance mode";

    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance::idle >())
                                << "Vending machine is in idle maintenance mode";
    // Load some goods
    EXPECT_TRUE(done(vm.process_event(events::load_goods{ 0, 10 })))
                                << "Vending machine consumes goods";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance::idle >())
                                << "Vending machine transits back to idle maintenance mode";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_EQ(10, vm.count());
    EXPECT_TRUE(done(vm.process_event(events::load_goods{ 1, 100 })))
                                << "Vending machine consumes goods";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::maintenance::idle >())
                                << "Vending machine transits back to idle maintenance mode";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_EQ(110, vm.count());
    EXPECT_FALSE(vm.prices_correct());

    // Try leave maintenance mode without setting prices
    EXPECT_FALSE(ok(vm.process_event(events::end_maintenance{})));

    // Set prices
    EXPECT_TRUE(done(vm.process_event(events::set_price{ 0, 10.0 })))
                                << "Set price for item 0";
    EXPECT_TRUE(done(vm.process_event(events::set_price{ 1, 5.0 })))
                                << "Set price for item 1";
    EXPECT_FALSE(vm.is_empty());
    EXPECT_TRUE(vm.prices_correct());
    // Leave maintenance mode
    EXPECT_EQ(result::process, vm.process_event(events::end_maintenance{}));
}

TEST(Vending, BuyItem)
{
    vending_machine vm{ goods_storage{
        { 0, { 10, 15.0f } },
        { 1, { 100, 5.0f } }
    }};

    EXPECT_FALSE(vm.is_empty()) << "Vending machine is empty";

    EXPECT_TRUE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is off";
    EXPECT_FALSE(vm.is_in_state< vending_def::on >())
                                << "Vending machine is not on";

    EXPECT_TRUE(done(vm.process_event(events::power_on{})))
                                << "Vending machine turns on correctly";

    EXPECT_FALSE(vm.is_in_state< vending_def::off >())
                                << "Vending machine is not off";
    EXPECT_TRUE(vm.is_in_state< vending_def::on >())
                                << "Vending machine is on";
    EXPECT_TRUE(vm.is_in_state< vending_def::on::serving >())
                                << "Vending machine is serving";

}

}  /* namespace vending */
