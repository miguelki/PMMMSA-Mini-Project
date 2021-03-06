// yex_bouche_nez.cpp�: d�finit le point d'entr�e pour l'application console.

#include "stdafx.h"
#include "OscOutboundPacketStream.h"
#include "IpEndpointName.h"
#include "OscTypes.h"
#include "UdpSocket.h"

#define NB_CLSF 4 // number of classifiers

// OSC stuff
#define ADDRESS "127.0.0.1"
#define PORT 7000
#define OUTPUT_BUFFER_SIZE 1024

// Create memory for calculations
CvMemStorage* storages[NB_CLSF] = {0, 0, 0, 0};

// Create a new Haar classifier
CvHaarClassifierCascade* cascade[NB_CLSF] = {0, 0, 0, 0};

// Colors for each feature : red - green - yellow
CvScalar colors[NB_CLSF] = {CV_RGB(255, 0, 0), CV_RGB(0, 255, 0), CV_RGB(0, 0, 255), CV_RGB(255, 255, 0)};

// Function prototype for detecting and drawing an object from an image
std::vector<int> detect_and_draw(IplImage* img);

// sound processing function
std::vector<float> process_values(std::vector<int> prev_a, std::vector<int> new_a);

// Create a std::string that contains the cascade name
const char *cascade_name[NB_CLSF]={"haar/face.xml", "haar/haarcascade_lefteye_2splits.xml", "haar/haarcascade_righteye_2splits.xml", "haar/mouth.xml" };

using namespace std;

//Point d'entr�e du programme
int main (int argc, char * const argv[]) {
	// OSC variables for data transmission to pure data
	UdpTransmitSocket transmitSocket( IpEndpointName( ADDRESS, PORT ) );

	DWORD start, end;

	char buffer[OUTPUT_BUFFER_SIZE];
	osc::OutboundPacketStream p( buffer, OUTPUT_BUFFER_SIZE );

	// Structure containing the previous areas
	std::vector<int> prev_a;
	prev_a.clear();

	// Structure for getting video from camera or avi
	CvCapture* capture = 0;
	int key = 'a';

	// Images to capture the frame from video or camera or from file
	IplImage *frame, *frame_copy = 0;

	// Used for calculations
	int optlen = strlen("--cascade=");  

	capture = cvCreateCameraCapture(CV_CAP_ANY);

	if (!capture) {
		cout << "Not able to open video stream, abort mission" << endl;
		return -1;
	}

	cvNamedWindow("result", 1 );

	// If loaded succesfully, then:
	if( capture ) {
		for(int i=0;i<NB_CLSF;i++) {
			cascade[i] = (CvHaarClassifierCascade*)cvLoad(cascade_name[i], 0, 0, 0 );

			// Allocate the memory storage
			storages[i] = cvCreateMemStorage(0);

			// Check whether the cascade has loaded successfully. Otherwise report and error and quit
			if( !cascade[i] ) {
				fprintf( stderr, "ERROR: Could not load classifier cascade %s\n", cascade_name[i]);
				return -1;
			}
		}

		// Capture from the camera.
		while( key != 'q' ) { 

			start = GetTickCount();

			// Capture the frame and load it
			frame = cvQueryFrame( capture );

			// If the frame does not exist, quit the loop
			if( !frame )
				break;

			// Allocate framecopy as the same size of the frame
			if( !frame_copy )
				frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
				IPL_DEPTH_8U, frame->nChannels );

			cvFlip( frame, frame_copy, 1 );

			// vecteur avec toutes les valeurs 
			std::vector<int> res = detect_and_draw(frame_copy);

			if (!(prev_a.empty()) && !(res.empty())) {
				std::vector<float> values = process_values(prev_a, res);

				for (unsigned int v = 0; v < res.size(); v++)
					cout << res[v] << " ";

				cout << "\nOutput values : " << endl;

				for (unsigned int v = 0; v < values.size(); v++)
					cout << values[v] << " ";

				// transmit values to pure data
				p.Clear();
				std::ostringstream tmp, tmp2;
				tmp << (int)values[0] ;
				tmp2 <<  values[1];

				cout << "\nmessage content : " << tmp.str().c_str() << tmp2.str().c_str() << endl;				
				p<< osc::BeginMessage("/test1") << tmp.str().c_str() << tmp2.str().c_str() <<osc::EndMessage;

				transmitSocket.Send( p.Data(), p.Size() );
				p.Clear();			
			}

			prev_a = res;

			// Wait for a while before proceeding to the next frame
			if( cvWaitKey( 1 ) >= 0 )
				break;
			end = GetTickCount();
			unsigned long tm = end - start;
				cout << "It took " << tm << " millisecs to process the frame" << endl;
				cout << "-------" << endl;	
		}
	}

	// Release the images, and capture memory
	for(int i=0;i<NB_CLSF;i++) {
		cvReleaseHaarClassifierCascade(&(cascade[i]));
		cvReleaseMemStorage(&storages[i]);
	}

	cvReleaseImage( &frame_copy );
	cvReleaseCapture( &capture );

	return 0;
}

std::vector<int> detect_and_draw(IplImage* img) {
	std::vector<int> feat_areas;
	feat_areas.clear();

	int nb_eyes = 0;
	int eye_area = 0;
	int mth_area = 0;
	int scale = 1;

	// Create a new image based on the input image
	IplImage* temp = cvCreateImage( cvSize(img->width/scale,img->height/scale), 8, 3 );

	// Clear the memory storage which was used before
	cvClearMemStorage( storages[0] );

	// Find whether the cascade is loaded, to find the faces. If yes, then:
	if( cascade )
	{
		// There can be more than one face in an image. So create a growable sequence of faces.
		// Detect the objects and store them in the sequence
		CvSeq* faces = cvHaarDetectObjects( img, cascade[0], storages[0],
			1.1, 30, CV_HAAR_DO_CANNY_PRUNING,
			cvSize(50, 50),cvSize(400,400) );

		// Loop the number of faces found.
		if (faces->total == 0) {
			cvShowImage( "result", img );
			return feat_areas;
		} else {
			/* get the first detected face */
			CvRect *face = (CvRect*)cvGetSeqElem(faces, 0);

			/* draw a red rectangle */
			cvRectangle(img,
				cvPoint(face->x, face->y),
				cvPoint(face->x + face->width,face->y + face->height),
				colors[0], 1, 8, 0);	

			/* reset buffers for eyes detection */
			cvClearMemStorage(storages[1]);
			cvClearMemStorage(storages[2]);

			/* Set the Region of Interest: estimate left eye position */
			cvSetImageROI(
				img,                    /* the source image */
				cvRect(
				face->x,            /* x = start from leftmost */
				face->y + (int)(face->height/5.5), /* y = a few pixels from the top */
				(int)face->width/2.0,        /* width = half of the face */
				(int)face->height/2.0    /* height = 1/3 of face height */
				)
				);
			/*cvResetImageROI(img);
			cvRectangle(
			img,
			cvPoint(face->x, face->y + (int)(face->height/5.5)), cvPoint(face->x + (int)face->width/2, face->y + (int)face->height/2.0),
			colors[1],
			1, 8, 0
			);
			*/
			/* detect the eyes */
			CvSeq *l_eyes = cvHaarDetectObjects(
				img,            /* the source image, with the estimated location defined */
				cascade[2],      /* the eye classifier */
				storages[2],        /* memory buffer */
				1.1, 30, CV_HAAR_DO_CANNY_PRUNING,     /* tune for your app */
				cvSize(20, 20), cvSize(80, 80)  /* minimum detection scale */
				);

			/* get eyes */
			if (l_eyes->total > 0) {
				CvRect *l_eye = (CvRect*)cvGetSeqElem(l_eyes, 0);


				/* draw a rectangle */
				cvRectangle(
					img,
					cvPoint(l_eye->x, l_eye->y), cvPoint(l_eye->x + l_eye->width, l_eye->y + l_eye->height),
					colors[1],
					1, 8, 0
					);

				eye_area += (l_eye->height*l_eye->width);

				nb_eyes++;
			}

			/* reset region of interest */
			cvResetImageROI(img);

			/* Set the Region of Interest: estimate left eye position */
			cvSetImageROI(
				img,                    /* the source image */
				cvRect(
				face->x + (int)(face->width/2.0), /* x = start from leftmost */
				face->y + (int)(face->height/5.5), /* y = a few pixels from the top */
				(int)face->width/2.0,        /* width = half of the face */
				(int)face->height/2.0    /* height = 1/3 of face height */
				)
				);

			CvSeq *r_eyes = cvHaarDetectObjects(
				img,            /* the source image, with the estimated location defined */
				cascade[1],      /* the eye classifier */
				storages[1],        /* memory buffer */
				1.1, 30, CV_HAAR_DO_CANNY_PRUNING,     /* tune for your app */
				cvSize(20, 20), cvSize(80, 80)  /* minimum detection scale */
				);

			if (r_eyes->total > 0) {
				CvRect *r_eye = (CvRect*)cvGetSeqElem(r_eyes, 0);

				/* draw a rectangle */								
				cvRectangle(
					img,
					cvPoint(r_eye->x, r_eye->y), cvPoint(r_eye->x + r_eye->width, r_eye->y + r_eye->height),
					colors[2],
					1, 8, 0
					);

				eye_area += (r_eye->height*r_eye->width);

				nb_eyes++;
			}

			/** Compute eye rectangle area : mean of sum of all eye areas **/
			if (nb_eyes >0)
				feat_areas.push_back((int) eye_area / nb_eyes);
			else
				feat_areas.push_back((int) eye_area);

			/* reset buffer for mouth detection */
			cvClearMemStorage(storages[3]);

			/* reset region of interest */
			cvResetImageROI(img);

			/* Set the Region of Interest: estimate mouth position */
			cvSetImageROI(
				img,                    /* the source image */
				cvRect(
				face->x,            /* x = start from leftmost */
				face->y + (int)(face->height*2.0/3.0), /* y = 2/3 of the face */
				face->width,        /* width = same width as the face */
				(int)face->height/3.0    /* height = 1/3 of face height */
				)
				);			

			// detect mouth
			CvSeq *mouths = cvHaarDetectObjects(
				img,            /* the source image, with the estimated location defined */
				cascade[3],      /* the eye classifier */
				storages[3],        /* memory buffer */
				1.1, 30, CV_HAAR_DO_CANNY_PRUNING,     /* tune for your app */
				cvSize(20, 20), cvSize(100, 80)  /* minimum detection scale */
				);

			if (mouths->total > 0) {

				// draw mouth rectangle
				CvRect *mouth = (CvRect*)cvGetSeqElem(mouths, 0);

				/* draw a rectangle */
				cvRectangle(
					img,
					cvPoint(mouth->x, mouth->y),
					cvPoint(mouth->x + mouth->width, mouth->y + mouth->height),
					colors[3],
					1, 8, 0
					);

				/** Compute mouth rectangle area**/
				mth_area = (int) mouth->width * mouth->height;
			} 

			feat_areas.push_back(mth_area);
			/* reset region of interest */
			cvResetImageROI(img);
		}
	}

	// Show image in window named "result"
	cvShowImage( "result", img );

	// Release temp image created.
	cvReleaseImage( &temp );

	return feat_areas;
}

std::vector<float> process_values(std::vector<int> prev_a, std::vector<int> new_a) {

	std::vector<float> res;
	res.clear();
	float tmp = 0;

	// mouth value : (prev_v + new_v)/2 
	if (new_a[1] != 0 && prev_a[1] != 0) {
		tmp = (float)prev_a[1] / (float)new_a[1];
		if (tmp > 1)
			tmp--;
	}
	res.push_back(3000*tmp);
	tmp = 0;

	// eyes value : offset previous value (increase or decrease) depending on the difference 
	if (new_a[0] != 0 && prev_a[0] != 0) {
		tmp = (float)prev_a[0] / (float)new_a[0];
		if (tmp > 1)
			tmp--;
	}

	char sz[64];
	sprintf(sz, "%.2lf\n", tmp);
	tmp = atof(sz);

	res.push_back(tmp);
	tmp = 0;

	return res;
}
