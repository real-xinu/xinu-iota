#ifndef  GLOBAL_H
#define  GLOBAL_H

extern int s;
extern int nnodes;
extern FILE *fp;
extern FILE *log_f;
extern char *path;
char *log_file_path;
extern char *scripts_path;
extern char *imgpath;
extern struct sockaddr_in si_other;
extern const char *SRV_IP;
extern char map_list[46][20];
extern char map_serv[15];
extern char ip_serv[15];
extern char map_brouter[15];
extern char array_token[2][20];
extern const char * const  cmdtab[];
extern int ncmd; 
extern char * ip_list[6];
#endif

