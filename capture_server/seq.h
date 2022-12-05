
/***************************************************************************
 * AESD Final Project
 * Author:  Chinmay Shalawadi 
 * Institution: University of Colorado Boulder
 * Mail id: chsh1552@colorado.edu
 * References: AESDSocket, Wikipedia, ChatGPT & stb header library
 ***************************************************************************/

#ifndef SEQ_
#define SEQ

void open_device(void);
void close_device(void);
void init_device(void);
void uninit_device(void);
void start_capturing(void);
void stop_capturing(void);
void take_picture(void);
unsigned char *get_processed_image_data();

#endif