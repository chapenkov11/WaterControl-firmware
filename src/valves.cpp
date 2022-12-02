// #include "valves.h"

// Mode mode = Mode::NORMAL;

// Valve<valve1_driver_in_1, valve1_driver_in_2, valve1_adc> valve;
// // Valve<valve2_power, valve2_direction, valve2_adc> valve2;

// ValveStatus valves_getStatus()
// {
//     if (valve.getStatus() == DONE)
//     {
//         return DONE;
//     }
//     else
//     {
//         return RUNNING;
//     }
// }

// ValvePosition valves_getPosition()
// {
//     if (valve.getPosition() == OPEN)
//     {
//         return OPEN;
//     }
//     else
//     {
//         return CLOSE;
//     }
// }

// void valves_setPosition(ValvePosition position)
// {
//     valve.setPosition(position);
// }

// void valves_run()
// {
//     valve.run();

//     /* вкл. преобразователь */
//     if (valve.getStatus() == RUNNING && !VALVE_POWER::IsSet())
//     {
//         VALVE_POWER::On();
//     }
//     else
//     {
//         VALVE_POWER::Off();
//     }
// }

// void valves_prevent_run()
// {
//     static int state = 0;

//     if (state == 0)
//     {
//         LOG("Pr0");
//         if (valve.getPosition() == OPEN)
//         {
//             valve.setPosition(CLOSE);
//             state = 1;
//         }
//         else
//         {
//             state = 2;
//         }
//     }
//     else if (state == 1)
//     {
//         LOG("Pr1");
//         valve.setPosition(OPEN);
//         state = 2;
//     }
//     else if (state == 2)
//     {
//         LOG("Pr2");
//         nextCheckValv = time + INTERVAL_CHECK_VALV;
//         mode = Mode::NORMAL;
//         state = 0;
//     }
// }