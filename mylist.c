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

//Nodo elemento de lista para el guardado de informacion de los archivos
/*
-En checksum se guarda el md5sum
-En *name_file se guarda el nombre del archivo al que se le realizara el md5sum
-En change se almacenara SI para denotar cambio en un archivo y NO para el contrario
*/
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

	Lista *lista;  //Lista que funcionara como base de datos para el guardado de los md5sum de los archivos

	char *tag;     //tag que nos permitira pasar mensajes a los usuarios
    int interval;  // intervalo de tiempo para la proxima revision

//concatena el contenido de s2 en s1
	char* concat(char *s1, char *s2){

		int tam = strlen(s1)+strlen(s2)+1;
	    char *result;
	    result = (char *)malloc(tam);
	    strcpy(result, s1);
	    strcat(result, s2);
	    return result;
	}

//Retorna la extension de un archivo
	 char *get_filename_ext( char *filename) {
	 	char *dot = strrchr(filename, '.');
	    if(!dot || dot == filename) return "";
	    	return dot + 1;
	}

//retorna el nombre de un archivo de una ruta
	char *get_filename_path( char *filename) {
	    char *dot = strrchr(filename, '/');
	    if(!dot || dot == filename) return "";
	    	return dot + 1;
	}

//obtiene palabras de un archivo
	 char  **ObtenerPalabras(char *nomArch,char **paragraph,int position){

	      paragraph = (char**)realloc(paragraph, (position+1)*sizeof(char*));
	      paragraph[position] = nomArch;

	 return paragraph;

	}

    //obtener el size de un archivo
    long long fsize(char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return (long long)st.st_size;

			syslog(LOG_ERR,"Error, No se pudo determinar el size del archivo");

    return -1;
	}


	/*Inicializar una lista*/
	void incializacion(Lista *lista){

	    lista->inicio = NULL;
	    lista->fin= NULL;
	    lista->tamano = 0;
	}

	/*Inserción en una lista vacía */
	void InsercionEnListaVacia(Lista *lista, char *check, char *name){

	    Elemento *nuevo_elemento;
	    if((nuevo_elemento = (Elemento *)malloc(sizeof(Elemento)))==NULL){
			syslog(LOG_ERR,"Fallo en reservar memoria de nodo");
	        exit(EXIT_FAILURE);
		}

	    if((nuevo_elemento->checksum = (char *)malloc(50*sizeof(char)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc check de insersion lista vacia");
	        exit(EXIT_FAILURE);
	    }
	    strcpy(nuevo_elemento->checksum, check);

	    if((nuevo_elemento->name_file = (char *)malloc(50*sizeof(char)))==NULL){
			syslog(LOG_ERR,"Fallo en malloc name de insersion lista vacia");
			exit(EXIT_FAILURE);
		}
	    strcpy(nuevo_elemento->name_file, name);

	    if((nuevo_elemento->change = (char *)malloc(10*sizeof(char)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc chance de insersion lista vacia");
	        exit(EXIT_FAILURE);
		}
	    strcpy(nuevo_elemento->change,"NO");

	    nuevo_elemento->siguiente = NULL;
	    lista->inicio = nuevo_elemento;
	    lista->fin = nuevo_elemento;
	    lista->tamano++;

	}

	/*Insercion al inicio de una lista*/
	void InsercionInicioLista(Lista *lista, char *check, char *name){

	    Elemento *nuevo_elemento;
	    if((nuevo_elemento = (Elemento *)malloc(sizeof(Elemento)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc de insersion inicio lista");
	        exit(EXIT_FAILURE);
	    }

	    if((nuevo_elemento->checksum = (char *)malloc(50*sizeof(char)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc check de insersion inicio lista");
	        exit(EXIT_FAILURE);
		}
	    strcpy(nuevo_elemento->checksum, check);

	    if((nuevo_elemento->name_file = (char *)malloc(50*sizeof(char)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc name de insersion inicio lista");
	        exit(EXIT_FAILURE);
		}
	    strcpy(nuevo_elemento->name_file, name);

	    if((nuevo_elemento->change = (char *)malloc(10*sizeof(char)))==NULL){
	        syslog(LOG_ERR,"Fallo en malloc de chance insersion inicio lista");
	        exit(EXIT_FAILURE);
		}
	    strcpy(nuevo_elemento->change,"NO");

	    nuevo_elemento->siguiente = lista->inicio;
	    lista->inicio=nuevo_elemento;
	    lista->tamano++;

	}

	/*visualizar lista entera*/
	void visualizacion(Lista *lista){

	    Elemento *actual;
	    actual = lista->inicio;
	    while(actual != NULL){
	    	printf("Lista: checksum : %s, file: %s, tam_file: %d , change: %s\n",actual->checksum,actual->name_file,strlen(actual->name_file),actual->change);
	        actual = actual->siguiente;
	    }
	}

	//consultar namefile lista
	int consultar_name_lista(Lista *lista,char *file){

	 	Elemento *actual;
	    actual = lista->inicio;
	    while(actual != NULL){
	    	   //verifico que el archivo este en la lista
	            if(strcmp(actual->name_file,file)==0){
	            	return 1;
	            }
	        actual = actual->siguiente;
	    }
	return 0;
	}

	//Procesamos el .pak
	char *proccesingPak(Lista *lista){

	    Elemento *actual;
	    actual = lista->inicio;
	    //armamos el archivo
	    time_t t = time(NULL);
	    struct tm tm = *localtime(&t);

	    char *year = (char *)malloc(4*sizeof(char));
	    char *month = (char *)malloc(2*sizeof(char));
	    char *day = (char *)malloc(2*sizeof(char));
	    char *hour = (char *)malloc(2*sizeof(char));
	    char *min = (char *)malloc(2*sizeof(char));
	    char *sec = (char *)malloc(4*sizeof(char));

	    int anio = tm.tm_year + 1900;
	    int mes = tm.tm_mon + 1;
	    sprintf( year , "%d", anio );
	    sprintf( month, "%d", mes );
	    sprintf( day, "%d", tm.tm_mday );
	    sprintf( hour, "%d", tm.tm_hour );
	    sprintf( min, "%d", tm.tm_min );
	    sprintf( sec, "%d", tm.tm_sec );

		struct stat st = {0};

		if (stat("/var/log/PROYECTO_SO_1", &st) == -1) {
			mkdir("/var/log/PROYECTO_SO_1", 0744);
		}

	    char *namepackage = "/var/log/PROYECTO_SO_1/";
	    namepackage = concat(namepackage,"logs_");
	    namepackage = concat(namepackage,day);
	    namepackage = concat(namepackage," ");
	    namepackage = concat(namepackage,month);
	    namepackage = concat(namepackage," ");
	    namepackage = concat(namepackage,year);
	    namepackage = concat(namepackage,"-");
	    namepackage = concat(namepackage,hour);
	    namepackage = concat(namepackage,":");
	    namepackage = concat(namepackage,min);
	    namepackage = concat(namepackage,":");
	    namepackage = concat(namepackage,sec);
	    namepackage = concat(namepackage,".pak");

	    int size;
	    char buffer[4000];
	    int fdest;
	    int wr;

	    	while(actual != NULL){
	        	if( strcmp(actual->change,"SI")==0){

	                fdest = open(namepackage, O_RDWR | O_CREAT | O_APPEND ,0666);

	                	if(fdest==-1){
	                		syslog(LOG_ERR,"Fallo en apertura de archivo .pak");
	                		exit(EXIT_FAILURE);
	                	}

	                	//agrego el nombre del archivo y el tamaño del archivo
	                	char *auxfile = "/var/log/";
	                	auxfile = concat(auxfile,actual->name_file);


	                	long long len = fsize(auxfile);


	                	char *name_truncate = (char *)malloc(32*sizeof(char));


	                	int ini = strlen(actual->name_file);

	                	name_truncate = concat( name_truncate, actual->name_file );

	                	int i;

	                	for(i=ini; i<32; i++){
							name_truncate = concat(name_truncate,"0");
						}


	                	char *size_file = (char *)malloc(60*sizeof(char));

	                	sprintf( size_file, "%lld", len);

	                	char *finalhead = "";

	                	finalhead = concat(finalhead,name_truncate);

	                	finalhead = concat(finalhead," ");

	                	finalhead = concat(finalhead,size_file);


	                	wr = write(fdest,finalhead,strlen(finalhead));

	                	wr = write(fdest,"\n",strlen("\n"));

	                	char *fsourceaux = "";
	                	fsourceaux = concat(fsourceaux,"/var/log/");


	                	fsourceaux = concat(fsourceaux,actual->name_file);


	                	int fsource = open(fsourceaux, O_RDONLY);

	                	if(fsource==-1){
	                		syslog(LOG_ERR,"Fallo apertura archivo fuente");
	               			exit(EXIT_FAILURE);
	                	}

	                	while((size = read(fsource,buffer,sizeof(buffer)))!=0){
	                	    wr = write(fdest,buffer,size-1);
	                	}

	                	wr = write(fdest,"\n",strlen("\n"));

	                	close(fsource);

	                	memset(name_truncate, 0, strlen(name_truncate));
	                	memset(size_file, 0, strlen(size_file));

	            }
	            actual = actual->siguiente;
	    	}

	 wr = write(fdest,"FIN\n",strlen("FIN\n"));
	 close(fdest);
	 memset(buffer, 0, 4000);
	return namepackage;
	}

	void set_change(Lista *lista){

	    Elemento *actual;
	    actual = lista->inicio;
	    while(actual != NULL){
	    	if(strcmp(actual->change,"SI")==0){
	        	strcpy(actual->change,"NO");
	        }
	          actual = actual->siguiente;
	    }
	}

	int file_change(Lista *lista){

	    Elemento *actual;
	    actual = lista->inicio;
	    while(actual != NULL){
	    	if(strcmp(actual->change,"SI")==0){
	        	return 1;
	        }
	          actual = actual->siguiente;
	    }
	return 0;
	}

	// checksum lista
	void set_checksum_lista(Lista *lista, char *file ,char *check){

	 	Elemento *actual;
	    actual = lista->inicio;
	    int val;
	    	while(actual != NULL){

	        	if(strcmp(actual->name_file,file)==0){

	            	val = strcmp(actual->checksum,check);

	            	if(val!=0){
	               		strcpy(actual->checksum,check);//seteo el checksum
	               		strcpy(actual->change,"SI");//seteo el flag de que el check cambio
					}

	            	val =-100;
	            }
	            actual = actual->siguiente;
	        }
	}

    /*Eliminar en el inicio de la lista*/
	int EliminarInicio(Lista *lista){

	    if(lista->tamano == 0)
	        return -1;
	    Elemento *sup_elemento;
	    sup_elemento = lista->inicio;
	    lista->inicio=lista->inicio->siguiente;
	    if(lista->tamano==1)
	        lista->fin = NULL;
	    free(sup_elemento->checksum);
	    free(sup_elemento->name_file);
	    free(sup_elemento);
	    lista->tamano--;
	    return 0;
	}


	/* destruir la lista */
	void destruir (Lista * lista){
	  while (lista->tamano > 0)
	  	EliminarInicio (lista);
	}
