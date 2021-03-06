/****************************************************************************
 *
 * This file is part of the ViSP software.
 * Copyright (C) 2005 - 2014 by INRIA. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * ("GPL") version 2 as published by the Free Software Foundation.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact INRIA about acquiring a ViSP Professional
 * Edition License.
 *
 * See http://team.inria.fr/lagadic/visp for more information.
 *
 * This software was developed at:
 * INRIA Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 * http://team.inria.fr/lagadic
 *
 * If you have questions regarding the use of this file, please contact
 * INRIA at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * This example demonstrates how to control the robot remotely in position and velocity.
 *
 * Authors:
 * Giovanni Claudio
 *
 *****************************************************************************/

/*! \example speech_recognition.cpp */
#include <iostream>
#include <string>

#include <alproxies/almemoryproxy.h>

#include <visp_naoqi/vpNaoqiRobot.h>


/*!

   Connect toRomeo robot, and apply some motion.
   By default, this example connect to a robot with ip address: 198.18.0.1.
   If you want to connect on an other robot, run:

   ./motion --ip <robot ip address>

   Example:

   ./motion --ip 169.254.168.230
 */
int main(int argc, const char* argv[])
{
  try
  {
    std::string opt_ip = "198.18.0.1";;

    if (argc == 3) {
      if (std::string(argv[1]) == "--ip")
        opt_ip = argv[2];
    }




    AL::ALMemoryProxy memProxy(opt_ip, 9559);


    unsigned int count = 0;
    unsigned int max_count = 10;
    float e_l = 0.0;
    float e_r = 0.0;
    float ratio = 0.0;
    float mean_e_l = 0.0;
    float mean_e_r = 0.0;


    while (count<max_count)
    {
      ratio = memProxy.getData("ALSoundProcessing/ratioRightLeft");
      e_l = memProxy.getData("ALSoundProcessing/leftMicEnergy");
      e_r = memProxy.getData("ALSoundProcessing/rightMicEnergy");

      mean_e_l += e_l;
      mean_e_r += e_r;

      //  std::cout << "leftMicEnergy" << e_l << std::endl;

      count++;

    }

    mean_e_l = mean_e_l/max_count;
    mean_e_r = mean_e_r/max_count;

    std::cout << "Mean Energy Left channel: " << mean_e_l << std::endl;
    std::cout << "Mean Energy right channel: " << mean_e_r << std::endl;


  }
  catch (const vpException &e)
  {
    std::cerr << "Caught exception: " << e.what() << std::endl;
  }
  catch (const AL::ALError &e)
  {
    std::cerr << "Caught exception: " << e.what() << std::endl;
  }



  return 0;
}

