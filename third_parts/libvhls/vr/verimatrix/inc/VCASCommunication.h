 #ifndef VRCOMPAT_H
 #define VRCOMPAT_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 extern int SetupVCASConnection(char * keyurl);
 extern int getvrkey(char* keyurl, uint8_t* keydat);
 
 #ifdef __cplusplus
 }
 #endif
 
 #endif
