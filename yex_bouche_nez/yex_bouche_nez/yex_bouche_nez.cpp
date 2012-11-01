// yex_bouche_nez.cpp : définit le point d'entrée pour l'application console.

#include "stdafx.h"

#define NB_CLSF 3 // number of classifiers

// Create memory for calculations
CvMemStorage* storages[NB_CLSF] = {0, 0, 0};

// Create a new Haar classifier
CvHaarClassifierCascade* cascade[NB_CLSF] = {0, 0, 0};

// Colors for each feature : red - green - yellow
CvScalar colors[NB_CLSF] = {CV_RGB(255, 0, 0), CV_RGB(0, 255, 0), CV_RGB(255, 255, 0)};

// Function prototype for detecting and drawing an object from an image
//bool detect_and_draw(IplImage* img, CvHaarClassifierCascade* cascade, CvScalar color, CvMemStorage* storage);
std::vector<std::string> detect_and_draw(IplImage* img);

// Create a std::string that contains the cascade name
const char *cascade_name[NB_CLSF]={"haar/face.xml", "haar/haarcascade_eye.xml", "haar/mouth.xml" };

using namespace std;

//Point d'entrée du programme
int main (int argc, char * const argv[]) {
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

			// Capture the frame and load it
			frame = cvQueryFrame( capture );

			// If the frame does not exist, quit the loop
			if( !frame )
				break;

			// Allocate framecopy as the same size of the frame
			if( !frame_copy )
				frame_copy = cvCreateImage( cvSize(frame->width,frame->height),
				IPL_DEPTH_8U, frame->nChannels );

			// Check the origin of image. If top left, copy the image frame to frame_copy. 
			if( frame->origin == IPL_ORIGIN_TL )
				cvCopy( frame, frame_copy, 0 );
			// Else flip and copy the image
			else
				cvFlip( frame, frame_copy, 0 );

			/*for(int i=0;i<NB_CLSF;i++) {
			// Call the function to detect and draw the face
			detect_and_draw(frame_copy,cascade[i], colors[i], storages[i]);	
			}
			*/
			std::vector<std::string> res = detect_and_draw(frame_copy);
			for (int v = 0; v < res.size(); v++)
				cout << res[v] << endl;
			cout << "-------" << endl;

			// Wait for a while before proceeding to the next frame
			if( cvWaitKey( 1 ) >= 0 )
				break;
		}
	}

	// Release the images, and capture memory
	for(int i=0;i<NB_CLSF;i++) {
		cvReleaseHaarClassifierCascade(&(cascade[i]));
		cvReleaseMemStorage(&storages[i]);
	}
	cvReleaseImage( &frame_copy );
	cvReleaseCapture( &capture );

	// If the capture is not loaded succesfully, then:
	return 0;

}

// Function to detect and draw any faces that is present in an image
//bool detect_and_draw( IplImage* img,CvHaarClassifierCascade* cascade, CvScalar color, CvMemStorage* storage)
//{
//	int scale = 1;
//
//	// Create a new image based on the input image
//	IplImage* temp = cvCreateImage( cvSize(img->width/scale,img->height/scale), 8, 3 );
//
//	// Create two points to represent the face locations
//	CvPoint pt1, pt2;
//	int i;
//
//	// Clear the memory storage which was used before
//	cvClearMemStorage( storage );
//
//	// Find whether the cascade is loaded, to find the faces. If yes, then:
//	if( cascade )
//	{
//		// There can be more than one face in an image. So create a growable sequence of faces.
//		// Detect the objects and store them in the sequence
//		CvSeq* faces = cvHaarDetectObjects( img, cascade, storage,
//			1.1, 30, CV_HAAR_DO_CANNY_PRUNING,
//			cvSize(50, 50),cvSize(400,400) );
//
//		// Loop the number of faces found.
//		for( i = 0; i < (faces ? faces->total : 0); i++ ) {
//			// Create a new rectangle for drawing the face
//			CvRect* r = (CvRect*)cvGetSeqElem( faces, i );
//
//			// Find the dimensions of the face,and scale it if necessary
//			pt1.x = r->x*scale;
//			pt2.x = (r->x+r->width)*scale;
//			pt1.y = r->y*scale;
//			pt2.y = (r->y+r->height)*scale;
//
//			// Draw the rectangle in the input image
//			cvRectangle( img, pt1, pt2, color, 3, 8, 0 );
//		}
//	}
//
//	// Show the image in the window named "result"
//	cvShowImage( "result", img );
//
//	// Release the temp image created.
//	cvReleaseImage( &temp );
//
//	if(i > 0)
//		return 1;
//	else
//		return 0;
//}

std::vector<std::string> detect_and_draw(IplImage* img) {
	std::vector<std::string> feat_nb;
	std::vector<std::string> feat_areas;
	feat_nb.clear();

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
			return feat_nb;
		} else {
			/* get the first detected face */
			CvRect *face = (CvRect*)cvGetSeqElem(faces, 0);

			/* draw a red rectangle */
			cvRectangle(img,
				cvPoint(face->x, face->y),
				cvPoint(face->x + face->width,face->y + face->height),
				colors[0], 1, 8, 0);	

			/** Add face rectangle area into std::vector **/
			ostringstream f_a;
			f_a << face->width*face->height;
			feat_nb.push_back(std::string("1"));
			feat_areas.push_back(f_a.str());

			/* reset buffer for the next object detection */
			cvClearMemStorage(storages[1]);

			/* Set the Region of Interest: estimate the eyes' position */
			cvSetImageROI(
				img,                    /* the source image */
				cvRect(
				face->x,            /* x = start from leftmost */
				face->y + (int)(face->height/5.5), /* y = a few pixels from the top */
				face->width,        /* width = same width as the face */
				(int)face->height/3.0    /* height = 1/3 of face height */
				)
				);

			/* detect the eyes */
			CvSeq *eyes = cvHaarDetectObjects(
				img,            /* the source image, with the estimated location defined */
				cascade[1],      /* the eye classifier */
				storages[1],        /* memory buffer */
				1.1, 30, CV_HAAR_DO_CANNY_PRUNING,     /* tune for your app */
				cvSize(20, 20), cvSize(60, 60)  /* minimum detection scale */
				);

			/** Add number of eyes detected into std::vector **/
			ostringstream e_nb;
			e_nb << eyes->total;
			feat_nb.push_back(e_nb.str());

			/* draw a rectangle for each detected eye */
			for( int e = 0; e < (eyes ? eyes->total : 0); e++ ) {
				/* get one eye */
				CvRect *eye = (CvRect*)cvGetSeqElem(eyes, e);

				/* draw a rectangle */
				cvRectangle(
					img,
					cvPoint(eye->x, eye->y), cvPoint(eye->x + eye->width, eye->y + eye->height),
					colors[1],
					1, 8, 0
					);

				/** Add eye rectangle area detected into std::vector **/
				ostringstream e_a;
				e_a << eye->height*eye->width;
				feat_areas.push_back(e_a.str());
			}

			/* reset buffer for the next object detection */
			cvClearMemStorage(storages[2]);

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
				cascade[2],      /* the eye classifier */
				storages[2],        /* memory buffer */
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
					colors[2],
					1, 8, 0
					);

				/** Add mouth rectangle area detected into std::vector **/
				ostringstream m_a;
				m_a << mouth->height*mouth->width;
				feat_nb.push_back(std::string("1"));
				feat_areas.push_back(m_a.str());

			} else {
				feat_nb.push_back(std::string("0"));
			}
			/* reset region of interest */
			cvResetImageROI(img);
		}
	}

	// Show image in window named "result"
	cvShowImage( "result", img );

	// Release temp image created.
	cvReleaseImage( &temp );

	feat_areas.insert(feat_areas.begin(), feat_nb.begin(), feat_nb.end());

	return feat_areas;
}


