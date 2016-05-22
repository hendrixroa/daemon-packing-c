#ifndef _LIB_LIST
//#define _LIB_LIST


	/*MANEJO DE CADENAS*/ 
	//concatena s2 en s1
	char* concat( char *s1, char *s2 );

	//Retorna la extension de filename
	char *get_filename_ext( char *filename ); 

	//obtiene el nombre de archivo de una ruta
	char *get_filename_path( char *filename );

	//Llena un arreglo con el contenido de un archivo 
	char  **ObtenerPalabras(char *nomArch,char **paragraph,int position);
	
	//obtener el tamanio de los archivos
	long long fsize(char *filename);


#endif
