/**
 *
 * This example demonstrates how to get images from the robot remotely, how
 * to detect a object with a target of four blobs on it and to graps it (from the side).
 * For this demo we use a box of tea and we track simultaneously all the 8 blobs (4 on the object and 4 on the hands) .
 *
 */


// Aldebaran includes.
#include <alproxies/altexttospeechproxy.h>

// ViSP includes.
#include <visp/vpDisplayX.h>
#include <visp/vpImage.h>
#include <visp/vpImageConvert.h>

#include <visp/vpDot2.h>
#include <visp/vpImageIo.h>
#include <visp/vpImagePoint.h>
#include <visp/vpFeaturePoint.h>
#include <visp/vpServo.h>
#include <visp/vpCameraParameters.h>
#include <visp/vpPixelMeterConversion.h>
#include <visp/vpMeterPixelConversion.h>
#include <visp/vpPlot.h>
#include <visp/vpFeatureBuilder.h>
#include <visp/vpCameraParameters.h>
#include <visp/vpXmlParserCamera.h>
#include <visp/vpXmlParserHomogeneousMatrix.h>
#include <visp/vpPose.h>

#include <iostream>
#include <string>
#include <list>
#include <iterator>

#include <visp_naoqi/vpNaoqiGrabber.h>
#include <visp_naoqi/vpNaoqiRobot.h>
#include <visp_naoqi/vpNaoqiConfig.h>
#define SAVE 0

#define USE_PLOTTER
#define L 0.015


using namespace AL;


bool computeCentroidBlob(const vpImage<unsigned char> &I,std::list<vpDot2> &blob_list,vpImagePoint &cog_tot,bool &init_done )
{
  vpImagePoint cog;
  cog_tot.set_uv(0,0);
  try
  {
    if (! init_done)
    {
      vpDisplay::flush(I);
      blob_list.clear();
      blob_list.resize(8);

      vpDisplay::displayCharString(I, vpImagePoint(10,10), "Click on the 8 blobs (Hand and Object) ", vpColor::red);

      for(std::list<vpDot2>::iterator it=blob_list.begin(); it != blob_list.end(); ++it)
      {

        (*it).setGraphics(true);
        (*it).setGraphicsThickness(2);
        (*it).initTracking(I);
        (*it).track(I);
        vpDisplay::flush(I);
        cog = (*it).getCog();
        cog_tot = cog_tot + cog;

      }

      cog_tot = cog_tot * ( 1.0/ (blob_list.size()) );
      init_done = true;
      std::cout << "init done: " << init_done << std::endl;
    }
    else
    {
      for(std::list<vpDot2>::iterator it=blob_list.begin(); it != blob_list.end(); ++it)
      {

        (*it).track(I);
        cog = (*it).getCog();
        cog_tot = cog_tot + cog;


      }

      // Compute the center of gravity of the object
      cog_tot = cog_tot * ( 1.0/ (blob_list.size()) );
    }
  }
  catch(...)
  {
    init_done = false;
    return false;
  }
  return true;

}


int main(int argc, char* argv[])
{
  std::string robotIp = "198.18.0.1";

  if (argc < 2) {
    std::cerr << "Usage: almotion_setangles robotIp "
              << "(optional default \"198.18.0.1\")."<< std::endl;
  }
  else {
    robotIp = argv[1];
  }


  /** Open Proxy for the speech*/
  AL::ALTextToSpeechProxy tts(robotIp, 9559);
  tts.setLanguage("English");
  const std::string phraseToSay = "Yes";
  bool speech = true;

  /** Open the grabber for the acquisition of the images from the robot*/
  vpNaoqiGrabber g;
  g.setFramerate(15);
  g.setCamera(0);
  g.open();


  vpCameraParameters cam = g.getCameraParameters(vpCameraParameters::perspectiveProjWithDistortion);
  std::cout << "Camera parameters: " << cam << std::endl;


  /** Create a new istance NaoqiRobot*/
  vpNaoqiRobot robot;

  robot.open();


  /** Initialization Visp Image, display and camera paramenters*/
  vpImage<unsigned char> I(g.getHeight(), g.getWidth());
  vpDisplayX d(I);
  vpDisplay::setTitle(I, "ViSP viewer");



  /** Load transformation between teabox and desired position of the hand to grasp it*/

  vpHomogeneousMatrix oMe_d;
  {
    vpXmlParserHomogeneousMatrix pm; // Create a XML parser
    std::string name_oMe_d =  "oMh_Small_Tea_Box1";

    char filename_[FILENAME_MAX];
    sprintf(filename_, "%s", VISP_NAOQI_GENERAL_M_FILE);

    if (pm.parse(oMe_d,filename_, name_oMe_d) != vpXmlParserHomogeneousMatrix::SEQUENCE_OK) {
      std::cout << "Cannot found the Homogeneous matrix named " << name_oMe_d<< "." << std::endl;
      return 0;
    }
    else
      std::cout << "Homogeneous matrix " << name_oMe_d <<": " << std::endl << oMe_d << std::endl;

  }

  // Position of the points on the target on the hand

  /** Point on the target*/
  int nbPoint =4 ;

  vpPoint point[nbPoint] ;
  point[0].setWorldCoordinates(-L,-L, 0) ;
  point[1].setWorldCoordinates(-L,L, 0) ;
  point[2].setWorldCoordinates(L,L, 0) ;
  point[3].setWorldCoordinates(L,-L,0) ;

  /** Position of the target points on the object*/
  vpPoint point_obj[nbPoint];
  point_obj[0].setWorldCoordinates(-L,-L, 0) ;
  point_obj[1].setWorldCoordinates(-L,L, 0) ;
  point_obj[2].setWorldCoordinates(L,L, 0) ;
  point_obj[3].setWorldCoordinates(L,-L,0) ;


  /** Initialization Visp blob*/
  std::list<vpDot2> blob_list;
  vpImagePoint cog_tot(0,0);


  bool init_done = false;


  /** Acquire image*/
  std::cout << "Click to start." << std::endl;
  while(1)
  {
    g.acquire(I);
    vpDisplay::display(I);
    vpDisplay::flush(I) ;
    if (vpDisplay::getClick(I, false))
      break;


  }
  std::cout << "Click into the blobs of the hand and then of the object (8 blobs in total)" << std::endl;


  // Detect the blobs on the object (in this case they will be 8 blobs)
  computeCentroidBlob(I, blob_list, cog_tot, init_done);

  vpHomogeneousMatrix cMo, cMh, cMhd ;
  vpImagePoint cog;
  /** Hand Pose */
  vpPose pose ;
  /** Object Pose */
  vpPose pose_obj ;


  pose.clearPoint();
  pose_obj.clearPoint();


      std::cout << "Number blobs: " << blob_list.size() << std::endl;

  unsigned int kk = 0;
  for (std::list<vpDot2>::iterator it=blob_list.begin(); it != blob_list.end(); ++it)
  {
    cog =  (*it).getCog();
    double x=0, y=0;
    std::cout << "Cog: " << cog << std::endl;
    vpPixelMeterConversion::convertPoint(cam, cog, x, y) ;
    if (kk <4)
    {
          std::cout << "Blob targ num" << kk << std::endl;
      point[kk].set_x(x) ;
      point[kk].set_y(y) ;
      pose.addPoint(point[kk]) ;
    }
    else
    {
          std::cout << "Blob obj num" << kk << std::endl;
      point_obj[kk-4].set_x(x) ;
      point_obj[kk-4].set_y(y) ;
      pose_obj.addPoint(point_obj[kk-4]) ;
    }

      kk++;
  }

  // compute the initial pose using Dementhon method followed by a non linear
  // minimisation method
  pose.computePose(vpPose::LAGRANGE, cMh) ;
  pose.computePose(vpPose::VIRTUAL_VS, cMh) ;
  std::cout << "Position Hand: " << std::endl << cMh << std::endl ;

  pose_obj.computePose(vpPose::LAGRANGE, cMo) ;
  pose_obj.computePose(vpPose::VIRTUAL_VS, cMo) ;
  std::cout << "Position Hand: " << std::endl << cMo << std::endl ;

  // Compute the desired position of the hand taking into account the off-set
  cMhd = cMo * oMe_d;

  vpDisplay::displayFrame(I, cMh, cam, 0.05, vpColor::none);
  vpDisplay::displayFrame(I, cMo, cam, 0.05, vpColor::none);
  vpDisplay::displayFrame(I, cMhd, cam, 0.05, vpColor::none);
  vpDisplay::flush(I) ;
  vpDisplay::getClick(I) ;



  // Sets the desired position of the visual feature
  vpHomogeneousMatrix cdMc ;
  cdMc = cMhd*cMh.inverse() ;
  vpFeatureTranslation t(vpFeatureTranslation::cdMc) ;
  vpFeatureThetaU tu(vpFeatureThetaU::cdRc); // current feature
  t.buildFrom(cdMc) ;
  tu.buildFrom(cdMc) ;



  /** Initialization Visual servoing */
  vpServo task; // Visual servoing task


  // We want to see a point on a point
  task.addFeature(t) ;   // 3D translation
  task.addFeature(tu) ; // 3D rotation


  task.setServo(vpServo::EYETOHAND_L_cVf_fVe_eJe);
  // Interaction matrix is computed with the desired visual features sd
  task.setInteractionMatrixType(vpServo::CURRENT, vpServo::PSEUDO_INVERSE);




  vpTRACE("Display task information " ) ;
  task.print() ;

  task.setLambda(0.10);


  //  // Set the proportional gain
  //  // - set the gain
  //  vpAdaptiveGain  lambda;
  //  lambda.initStandard(2, 0.2, 50);

  //  task.setLambda(lambda) ;

  vpColVector q_dot;


  // Constant transformation Target Frame to LArm end-effector (LWristPitch)
  vpHomogeneousMatrix oMe_LArm;
  for(unsigned int i=0; i<3; i++)
    oMe_LArm[i][i] = 0; // remove identity
  oMe_LArm[0][0] = 1;
  oMe_LArm[1][2] = 1;
  oMe_LArm[2][1] = -1;

  oMe_LArm[0][3] = -0.045;
  oMe_LArm[1][3] = -0.04;
  oMe_LArm[2][3] = -0.045;

  /** Load transformation between HeadRoll and CameraLeft*/
   vpHomogeneousMatrix eMc = g.get_eMc();



  // Motion

  std::vector<std::string> jointNames =  robot.getBodyNames("LArm");
  jointNames.pop_back(); // Delete last joints LHand, that we don't consider in the servo
  const unsigned int numJoints = jointNames.size();

  std::cout << "The " << numJoints << " joints of the Arm:" << std::endl << jointNames << std::endl;

  // Declarate Jacobian
  vpMatrix eJe_LArm;
  vpVelocityTwistMatrix oVe_LArm(oMe_LArm);
  vpMatrix oJo; // Jacobian in the target (=object) frame
  vpHomogeneousMatrix torsoMlcam_visp;
  vpHomogeneousMatrix torsoMo;
  //Set the stiffness
  robot.setStiffness(jointNames, 1.f);


  double tinit = 0; // initial time in second

  robot.getProxy()->openHand("LHand");

  vpImage<vpRGBa> O;

#ifdef USE_PLOTTER
  // Create a window (800 by 500) at position (400, 10) with 3 graphics
  vpPlot graph(2, 800, 500, 400, 10, "Curves...");
  // Init the curve plotter
  graph.initGraph(0, numJoints); // q_dot
  graph.initGraph(1, 6); // s-s*
  graph.setTitle(0, "Joint velocities");
  graph.setTitle(1, "Error s-s*");
  for(unsigned int i=0; i<numJoints; i++)
    graph.setLegend(0, i, jointNames[i].c_str());
  graph.setLegend(1, 0, "x");
  graph.setLegend(1, 1, "y");
#endif

  unsigned int iter = 0;
  while(1)
  {
    double time = vpTime::measureTimeMs();

#if 0
    showImages(camProxy,clientName, I);
    if(vpDisplay::getClick(I, false)) {
      vpImageIo::write(I, "/tmp/I.png");
    }
  }
#else
    try
    {
      g.acquire(I);
      vpDisplay::display(I);



      bool tracking_status = computeCentroidBlob(I, blob_list, cog_tot, init_done);

      if (! init_done)
        tinit = vpTime::measureTimeSecond();

      if (init_done && (tracking_status == true)) {

        pose.clearPoint();
        pose_obj.clearPoint();

        unsigned int kk = 0;
        for (std::list<vpDot2>::iterator it=blob_list.begin(); it != blob_list.end(); ++it)
        {
          cog =  (*it).getCog();
          double x=0, y=0;
          vpPixelMeterConversion::convertPoint(cam, cog, x, y) ;
          if (kk <4)
          {
            point[kk].set_x(x) ;
            point[kk].set_y(y) ;
            pose.addPoint(point[kk]) ;
          }
          else
          {
            point_obj[kk-4].set_x(x) ;
            point_obj[kk-4].set_y(y) ;
            pose_obj.addPoint(point_obj[kk-4]) ;
          }

            kk++;
        }

        // compute pose
        pose.computePose(vpPose::VIRTUAL_VS, cMh) ;
        //std::cout << "Position Hand: " << std::endl << cMh << std::endl ;

        pose_obj.computePose(vpPose::VIRTUAL_VS, cMo) ;
        //std::cout << "Position Hand: " << std::endl << cMo << std::endl ;

        // Compute the desired position of the hand taking into account the off-set
        cMhd = cMo * oMe_d;

        vpDisplay::displayFrame(I, cMh, cam, 0.05, vpColor::none);
        vpDisplay::displayFrame(I, cMo, cam, 0.05, vpColor::none);
        vpDisplay::displayFrame(I, cMhd, cam, 0.05, vpColor::none);

        cdMc = cMhd*cMh.inverse() ;
        t.buildFrom(cdMc) ;
        tu.buildFrom(cdMc) ;


        //** Set task eJe matrix
        eJe_LArm = robot.get_eJe("LArm");
        oJo = oVe_LArm * eJe_LArm;
        task.set_eJe(oJo);


        vpHomogeneousMatrix torsoMHeadRoll(robot.getProxy()->getTransform("HeadRoll", 0, true));


        torsoMlcam_visp = torsoMHeadRoll *eMc;


        vpVelocityTwistMatrix cVtorso(torsoMlcam_visp.inverse());
        task.set_cVf( cVtorso );

        //** Set task fVe matrix
        // get the torsoMe_LArm tranformation from NaoQi api

        vpHomogeneousMatrix torsoMLWristPitch(robot.getProxy()->getTransform("LWristPitch", 0, true));
        std::cout << "Torso M LWristPitch:\n" << torsoMLWristPitch << std::endl;


        torsoMo = torsoMLWristPitch * oMe_LArm.inverse();
        std::cout << "torso M object :\n" << torsoMo << std::endl;

        vpVelocityTwistMatrix torsoVo(torsoMo);
        task.set_fVe( torsoVo );

        q_dot = task.computeControlLaw(vpTime::measureTimeSecond() - tinit);




#ifdef USE_PLOTTER
        graph.plot(0, iter, q_dot); // plot joint velocities applied to the robot
        graph.plot(1, iter, task.getError()); // plot error vector s-s*
        iter++;
#endif

        task.print();


        std::cout << "q dot: " << q_dot.t() << " in deg/s: "
                  << vpMath::deg(q_dot[0]) << " " << vpMath::deg(q_dot[1]) << std::endl;

       // robot.setVelocity(jointNames, q_dot);
        robot.setVelocity(jointNames, q_dot);




        vpDisplay::displayFrame(I, torsoMlcam_visp.inverse()*torsoMLWristPitch, cam, 0.04, vpColor::green);


        vpDisplay::flush(I) ;
        //vpTime::sleepMs(20);


      }
      else {
        std::cout << "Stop the robot..." << std::endl;
        robot.stop(jointNames);

      }


    }
    catch (const AL::ALError& e)
    {
      std::cerr << "Caught exception " << e.what() << std::endl;
    }

    if (vpDisplay::getClick(I, false))

    {
      q_dot = 0.0 * q_dot;
      robot.setVelocity(jointNames, q_dot);

      break;
    }

    vpDisplay::flush(I);
    vpDisplay::getImage(I, O);
    std::cout << "Loop time: " << vpTime::measureTimeMs() - time << std::endl;
  }

  // Grasping

  //robot.stop(jointNames);

  std::string nameChain = "LArm";

  std::cout << "Click to Graps" << std::endl;
  vpDisplay::getClick(I);
  //robot.getProxy()->closeHand("LHand");
  robot.getProxy()->setStiffnesses("LHand", 1.0f);
  AL::ALValue angle = 0.15;
  robot.getProxy()->setAngles("LHand",angle,0.15);

  std::cout << "Click to take the object " << std::endl;
  vpDisplay::getClick(I);

  std::vector<float> handPos = robot.getProxy()->getPosition(nameChain, 0, false);
  handPos[2] =  handPos[2] + 0.07;
  robot.getProxy()->setPositions(nameChain,0,handPos,0.05,7);

  std::cout << "Click to put back the object " << std::endl;
  vpDisplay::getClick(I);

  handPos = robot.getProxy()->getPosition(nameChain, 0, false);
  handPos[2] =  handPos[2] - 0.06;
  robot.getProxy()->setPositions(nameChain,0,handPos,0.05,7);

  std::cout << "Click to Open the Hand" <<  std::endl;
  vpDisplay::getClick(I);

  //robot.getProxy()->openHand("LHand");


  robot.getProxy()->setStiffnesses("LHand", 1.0f);
  angle = 1.0f;
  robot.getProxy()->setAngles("LHand",angle,1.0);


  std::cout << "Click to Stop the demo" << std::endl;
  vpDisplay::getClick(I);

  std::cout << "The end: stop the robot..." << std::endl;
  robot.getProxy()->killMove();
  robot.stop(jointNames);
  task.kill();



#endif

  return 0;
}

