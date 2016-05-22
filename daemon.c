#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "mylist.h"

	pid_t pmd5sum;
	pid_t pgzip;

	char *tag;
    int interval;

	void manejadorDeSenales(pid_t process);


	typedef struct ElementoLista{
	    char *checksum;
	    char *name_file;
	    char *change;
	    struct ElementoLista *siguiente;
	}Elemento;

	typedef struct ListaIdentificar{
	    Elemento *inicio;
	    Elemento *fin;
	    int tamano;
	}Lista;

	Lista *lista;

	void hijo(int fds[2],char *buffer)
{
	char *fil = "";
	fil = concat(fil,"/var/log/");
	fil = concat(fil,buffer);
	close(fds[0]);
	dup2(fds[1], STDOUT_FILENO);
	close(fds[1]);
	execlp("md5sum", "md5sum",fil, NULL);
	syslog(LOG_ERR,"Error, fallo en execlp");
	exit(EXIT_FAILURE);
}

void comprimir(char *namepak)
{
  execlp("gzip","gzip",namepak, NULL);
  syslog(LOG_ERR,"Error, fallo en execlp");
  exit(EXIT_FAILURE);
}

void papa(int fds[2])
{
	int fd, numbytes;
	char buf[1000];
	close(fds[1]);

	numbytes = read(fds[0],buf,sizeof(buf));

	if (numbytes == -1) {
		syslog(LOG_ERR,"Fallo read de tuberia");
		exit(EXIT_FAILURE);
	}
     int i=0;
     char *check,*namef;
     char *aux = &buf[i];
     int index = strlen(aux);
     aux[index-1] = '\0';
     check = strsep(&aux," ");
     namef = strsep(&aux,"\0");
     char *namefile = get_filename_path(namef);
     //consulto si el namefile esta en la lista
     int existe = consultar_name_lista(lista,namefile);
    
     if(existe==0){ //es decir, no esta en la lista, lo agrego
     		if (lista->tamano == 0){
            	InsercionEnListaVacia(lista,check,namefile);
	 		}else{
            	InsercionInicioLista (lista,check,namefile);
	 		}
	}
   set_checksum_lista(lista,namefile,check);
  
	close(fds[0]);
    memset(buf, 0, 1000);
    memset(check, 0, strlen(check));
    memset(namef, 0, strlen(namef));

}

	int main(void){

		char *path = "/etc/proyecto_so_1/proy1.ini";
		char *line = NULL;
		size_t len = 0;
		ssize_t reading;
		openlog("ALERT_DAEMON", LOG_PID, LOG_DAEMON);

	 	FILE *fp;

	 	fp = fopen(path,"r");

	 	if (fp == NULL){
			syslog(LOG_ERR,"Fallo en la lectura del archivo de conf proy1.ini");
			closelog();
	    	exit(EXIT_FAILURE);
	      	//notificar por medio de syslog que no se encontro el proy1.ini
	 	}else{
			 	while(!feof(fp)){

	            	while ((reading = getline(&line, &len, fp)) != -1) {

	                	if(line[0] != ';'){

	                  		if(line[0] != '[' && line != NULL){
	                  			//printf("Linea %s\n",line);
	                  			char *buf;
	                  			buf = strsep(&line,"=");

	                  			if((strcmp(buf,"log_tag"))==0){
	                    			tag = strsep(&line,"\0");
	                  	
	                  			}

	                  			if((strcmp(buf,"interval"))==0){
	                    			buf = strsep(&line,"\0");
	                    			interval = atoi(buf);
	                  			
	                  			}
	                		}
	         			}
	       			}
	     		}
		}

		openlog(tag, LOG_PID, LOG_DAEMON);

		if ((lista = (Lista *) malloc (sizeof (Lista))) == NULL){
			syslog(LOG_ERR,"Error, no se pudo inicializar la lista");
	    	exit(EXIT_FAILURE);
	   	}

	   	incializacion(lista);




		while(1==1){

	  		sleep(interval);
	  		char *dir="/var/log";
	  		struct dirent *dp;
	  		DIR *fd;
	  		char  *val,**paragraph;
	  		paragraph = (char**)malloc(1*sizeof(char*));
	  		int position=0;

	  		if ((fd = opendir(dir)) == NULL) {
				
				syslog(LOG_INFO,"Error, no se pudo abrir el directorio /var/log");
	    		closelog();
	    		return;
	  		}

	  		while ((dp = readdir(fd)) != NULL) {

	 			if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
	    		continue;    /* skip self and parent */

	 			char *aux = get_filename_ext(dp->d_name);
	  			// printf("Val--------> %d type dir  %d  type file regular  %d\n ",dp->d_type,DT_DIR,DT_REG);
	   			if(dp->d_type == DT_REG){

	       			if(strcmp(aux,"gz")!=0){
						//agregar a la lista
	        			paragraph = ObtenerPalabras(dp->d_name,paragraph,position);
	        			position++;
	       			}
	   			}
	 		}


					(void) signal(SIGUSR1,manejadorDeSenales);
					int pos=0;

					//printf("Num pal %d\n",position);
					for(pos; pos < position; pos++){

						int i, fds[2];

						if (pipe(fds) == -1) {
							syslog(LOG_ERR,"Error, no se pudo crear la tuberia anonima");
							exit(EXIT_FAILURE);
						}

					 

							pmd5sum = fork();
							if (pmd5sum == 0) {

								hijo(fds,paragraph[pos]);
								kill(pmd5sum,SIGKILL);

							} else if (pmd5sum > 0) {
								/* tratamiento del padre */

					            papa(fds);
					            close(fds[0]);
						        close(fds[1]);
								kill(pmd5sum,SIGKILL);

							} else if (pmd5sum == -1) {

								syslog(LOG_ERR,"Error, fallo en la llamada fork");
								exit(EXIT_FAILURE);

							}

					    	/* tratamiento del padre una vez lanzado su hijo. */
					    	pmd5sum = wait(NULL);

							while (pmd5sum > 0) {
								pmd5sum = wait(NULL);
							}

							if (pmd5sum == -1 && errno != ECHILD) {
								syslog(LOG_ERR,"Error, fallo en la llamada wait");
								exit(EXIT_FAILURE);
							}

					}//fin for que crea hijos
                     // libero el paragragh
                     memset(paragraph,0,sizeof(char*)*position);
					 // visualizacion(lista);
					 // verifico si algun archivo cambio llamando a cambio ?
					char *namepak;
					int cambio = file_change(lista);

					if(cambio==1){//algun archivo cambio
						//procesar .pak
						namepak  = (char*)proccesingPak(lista);
					  	set_change(lista);

					  	pgzip = fork();
					    if (pgzip == 0) {

					          comprimir(namepak);
					          kill(pgzip,SIGKILL);

					    } else if (pgzip > 0) {
					      /* tratamiento del padre */

					    pgzip = wait(NULL);

					    while (pgzip > 0) {
					    	pgzip = wait(NULL);
					 	}


					    }else if (pgzip == -1) {
					    	syslog(LOG_ERR,"Error, fallo en fork");
					      	exit(EXIT_FAILURE);
					    }

					  	if (pgzip == -1 && errno != ECHILD) {
					    	syslog(LOG_ERR,"Error, fallo en wait");
					   		exit(EXIT_FAILURE);
					  	}

					}//FIN IF CAMBIO

	  	}//FIN WHILE TRUE
	}//FIN MAIN


	void manejadorDeSenales(pid_t process){

		kill(process,SIGTERM);

	}
