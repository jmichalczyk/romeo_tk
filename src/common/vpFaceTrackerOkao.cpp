
#include <vpFaceTrackerOkao.h>
#include <visp/vpImageConvert.h>
#include <alvision/alvisiondefinitions.h>

vpFaceTrackerOkao::vpFaceTrackerOkao(std::string ip, int port) :m_proxy(ip, port), m_mem_proxy(ip, port),
  m_scores(), m_image_height(240), m_image_width(320)

{

  // Start the face recognition engine
  const int period = 50;
  m_proxy.subscribe("Face", period, 0.0);
  m_proxy.setResolution(AL::kQVGA);
  m_proxy.enableTracking(true);
  m_proxy.enableRecognition(true);

  m_previuos_cog.set_uv(m_image_width / 2, m_image_height/2);

}

vpFaceTrackerOkao::~vpFaceTrackerOkao()
{
  m_proxy.unsubscribe("Face");
}

/*!
   Allows to detect a face in the image. When more than one face is detected, faces are sorted from largest to smallest.

   \return true if one or more faces are found, false otherwise.

   The number of detected faces is returned using getNbObjects().
   If a face is found the functions getBBox(), getCog() return some information about the location of the face.

   The largest face is always available using getBBox(0) or getCog(0).
 */

bool vpFaceTrackerOkao::detect()
{
  m_message.clear();
  m_polygon.clear();
  m_nb_objects = 0;
  m_faces.clear();
  m_scores.clear();

  bool target_found = false;
  AL::ALValue result = m_mem_proxy.getData("FaceDetected");
  //-- Detect faces

  if (result.getSize() >=2)
  {
    //std::cout << "face" << std::endl;
    AL::ALValue info_face_array = result[1];
    target_found = true;
    double min_dist = m_image_width*m_image_height;
    unsigned int index_closest_cog = 0;
    vpImagePoint closest_cog;
    for (unsigned int i = 0; i < info_face_array.getSize()-1; i++ )
    {
      //Extract face info
      // Face Detected [1]/ First face [0]/ Shape Info [0]/ Alpha [1]
      float alpha = result[1][i][0][1];
      float beta = result[1][i][0][2];
      float sx = result[1][i][0][3];
      float sy = result[1][i][0][4];

      std::string name = result[1][i][1][2];
      float score = result[1][i][1][1];

      std::ostringstream message;
      if (score > 0.6)
        message << name;
      else
        message << "Unknown";

      m_message.push_back( message.str() );
      m_scores.push_back(score);
      // sizeX / sizeY are the face size in relation to the image
      float h = m_image_height * sx;
      float w = m_image_width * sy;

      // Center of face into the image
      float x = m_image_width / 2 - m_image_width * alpha;
      float y = m_image_height / 2 + m_image_height * beta;

      vpImagePoint cog(x,y);
      double dist = vpImagePoint::distance(m_previuos_cog,cog);

      if (dist< min_dist)
      {
        closest_cog = cog;
        index_closest_cog = i;
        min_dist = dist;
      }

      std::vector<vpImagePoint> polygon;
      double x_corner = x - h/2;
      double y_corner = y - w/2;

      polygon.push_back(vpImagePoint(y_corner  , x_corner  ));
      polygon.push_back(vpImagePoint(y_corner+w, x_corner  ));
      polygon.push_back(vpImagePoint(y_corner+w, x_corner+h));
      polygon.push_back(vpImagePoint(y_corner  , x_corner+h));

      m_polygon.push_back(polygon);
      m_nb_objects ++;

    }

    if (index_closest_cog !=0)
      std::swap(m_polygon[0], m_polygon[index_closest_cog]);
    m_previuos_cog = closest_cog;
  }

  return target_found;
}


bool vpFaceTrackerOkao::detect(const vpImage<unsigned char> &I)
{
  return detect();
}

float vpFaceTrackerOkao::getScore(unsigned int i) const
{
  return m_scores[i];
}

/*!
   Remove all learned faces from the database.

   \return true if the operation succeeded
 */
bool vpFaceTrackerOkao::clearDatabase()
{
  return m_proxy.clearDatabase();
}

/*!
   Delete from the database all learned faces corresponding to the specified person.

  \param:	name – The name of the person to forget
  \return: true if the operation succeeded

 */
bool vpFaceTrackerOkao::forgetPerson(const std::string& name)
{
  return m_proxy.forgetPerson(name);
}

