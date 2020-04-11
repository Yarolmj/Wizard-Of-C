
//Using libs SDL, glibc
#include <SDL.h>	//SDL version 2.0
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>



#define SCREEN_WIDTH 832	//window height
#define SCREEN_HEIGHT 640	//window width

#define len(x) (sizeof(x) / sizeof((x)[0]))

//function prototypes
//initilise SDL
int init(int w, int h, int argc, char *args[]);
static void Spawn_Enemy(int);
void Create_Portals();
void Radar();


//Estructura usada por el sistema de collisiones
typedef struct Coll_array
{
	int array[4]; //Cuatro posibles direcciones de choque para cada objeto
} Coll_array;

typedef struct Object
{
	char *type; //Define el tipo del objeto, ejemplo: muro,player,enemy,shot
	int id; //Id unica por cada tipo de objeto, usada por los disparos para saber si un objeto ya disparó

	int last_dir; //Ultima direccion en la que se movió el objeto
	int enemy_cont; //Para las intersecciones, hace que los enemigos se muevan un titleset una vez deciden la direccion de avance

	char *from_type; //El tipo del que proviene el disparo
	int from_id; //Para la deteccion de choques con disparos


	int Alive;   //Determina si el objeto esta activo o no
	int enemy_type; //Tipo de enemigo. 1 = BURWOR, 2 = GARWOR, 3 = THORWOR, 4 = WORLUK, 5 = WIZARD
	int invisible_timer; //Para el GARWOR y THORWOR

	int x,y;      //Posicion del objeto
	int w,h;	  //Describe las dimensiones de una caja alrededor del objeto
	int AnimFrame; //Para las animaciones de las entidades
	SDL_Surface *surface; //Superficie con la textura de la entidad
	Uint32 Color; //Color de objeto, solo para formas simples
} Object;

//Estructura para cargar las texturas de las entidades de forma rapida y eficiente(memoria)
typedef struct surfs_array
{
	SDL_Surface *surfaces[4];
} surfs_array;

// Program globals
#define initial_enemy_number 6 //Numero maximo de enemigos en pantalla
#define tile_size 64 //Unidad base de espacio del juego
#define FileY 16 //Tamaño vertical del archivo de mapa
#define FileX 7 //Tamaño horizontal del archivo de mapa

int frame = 0; //Cuadro actual renderizado por el programa
int frames_quieto = 0; //Cuadros en los que se ha quedado quieto el jugador
int state = 0; //Estado progreso del juego
int respawn_timer = 0; //Para controllar la animacion de respawneo y evitar morir instantaneamente al reaparecer


static Object Enemigos[initial_enemy_number]; //Arreglo donde se guardan todas las entidades enemigas
static Object Muros[FileX*FileY]; //Arreglo donde se guardan todos lo muros del mapa que actualmente se ve en pantalla
static Object PortalL; //Entidad del portal izquierdo
static Object PortalR;	//Entidad del portal derecho
static Object *PortalLWall; //Colisionador portal izquierdo
static Object *PortalRWall; //Colisionador portal derecho

static Object Shots[initial_enemy_number+1]; //Almacena todos los proyectiles, tanto del jugador como del enemigo
static Object jugador; //Objeto que representa al jugador en el programa

static surfs_array player_s; //Texturas de jugador
static surfs_array blue_s; //Texturas del BURWOR
static surfs_array red_s; //Texturas del THORWOR
static surfs_array yellow_s; //Texturas del GARWOR
static surfs_array fly_s; //Texturas del WORLUK
static surfs_array wizard_s; //Texturas del Wizard of Wor

int GOD_MODE = 0; //PARA CONTROLAR EL MODO DIOS


int width, height;		//used if fullscreen

SDL_Window* window = NULL;	//The window we'll be rendering to
SDL_Renderer *renderer;		//The renderer SDL will use to draw to the screen

//surfaces
static SDL_Surface *title_screen; //Textura 1 de la pantalla de inicio
static SDL_Surface *press_enter; //Textura 2 de la pantalla de inicio
static SDL_Surface *screen; //Textura 1 del fondo
static SDL_Surface *Font_map; //Textura con todas las letras usadas
static SDL_Surface *Player_Life_surf; //Textura para las vidas presentadas en la UI

//Color data
Uint32 Blue;
Uint32 Red;
Uint32 Yellow;
Uint32 Black;
Uint32 White;

//Map data
char Map1_Matrix[FileY][FileX]; //Mapa 1 traducido y almacenado despues de leer el Archivo Map1.txt
char Map2_Matrix[FileY][FileX]; //Mapa 2 traducido y almacenado despues de leer el Archivo Map2.txt
char Map3_Matrix[FileY][FileX]; //Mapa 3 traducido y almacenado despues de leer el Archivo Map3.txt

//textures
SDL_Texture *screen_texture;


//Game Stats
int Level = 4; // Nivel cargado actualmente, 4 = Ninguno
int player_lifes; //Vidas del jugador
int actual_enemy_number = 0; //Numero de enemigos vivos
int score = 0; //Puntuacion
int killed_enemy_cont = 0; //Enemigos destruidos
int killed_enemy[5] = {0,0,0,0,0}; //Para el Game_Manager, enemigos eliminados por nivel
int dificultad; //Maneja la velocidad de los enemigos
int max_dificultad = 6; //Limite de velocidad
int type[5] = {0,0,0,0,0}; //Enemigos generados por nivel


//Carga los 3 mapas
void Load_Maps(){
	FILE *map_file = fopen("Maps/Map1.txt","r");
	for (int i = 0; i < FileY; ++i)
	{
		fscanf(map_file,"%s\n",Map1_Matrix[i]);
	}
	printf("Cargado mapa 1\n");
	map_file = fopen("Maps/Map2.txt","r");
	for (int i = 0; i < FileY; ++i)
	{
		fscanf(map_file,"%s\n",Map2_Matrix[i]);
	}
	printf("Cargado mapa 2\n");
	map_file = fopen("Maps/Map3.txt","r");
	for (int i = 0; i < FileY; ++i)
	{
		fscanf(map_file,"%s\n",Map3_Matrix[i]);
	}
	printf("Cargado mapa 3\n");

}

//Se encarga de la mecanica de reaparicion
void respawn(int timer){
	//En cuanto el jugador muere timer = 1920 F = 8 S * 240 F/S
	if(timer == 1920){
		//Resetea la posicion del jugador
		jugador.x = 708;
		jugador.y = 452;
		jugador.last_dir = 2;
	}
	//Cuando termina el temporalizador
	if(timer < 64){
		//Desplaza al jugador hacia arriba
		if(jugador.y > 388){
			jugador.y--;
		}
	}
	//Reduce el temporalizador
	if(respawn_timer != 0){
		respawn_timer--;
	}
}

//Inicializa la mayoria de parametros del juego
static void init_game() {
	//Cargo los mapas
	Load_Maps();
	//Cargo las fuentes
	Font_map = SDL_LoadBMP("Font_maps/Font_map.bmp");

	//Cargo las texturas de entidades
	Player_Life_surf = SDL_LoadBMP("Sprites/player/Life_Surf.bmp");

	player_s.surfaces[0] = SDL_LoadBMP("Sprites/player/Up.bmp");
	player_s.surfaces[1] = SDL_LoadBMP("Sprites/player/Down.bmp");
	player_s.surfaces[2] = SDL_LoadBMP("Sprites/player/Left.bmp");
	player_s.surfaces[3] = SDL_LoadBMP("Sprites/player/Right.bmp");


	blue_s.surfaces[0] = SDL_LoadBMP("Sprites/blue/Up.bmp");
	blue_s.surfaces[1] = SDL_LoadBMP("Sprites/blue/Down.bmp");
	blue_s.surfaces[2] = SDL_LoadBMP("Sprites/blue/Left.bmp");
	blue_s.surfaces[3] = SDL_LoadBMP("Sprites/blue/Right.bmp");

	red_s.surfaces[0] = SDL_LoadBMP("Sprites/red/Up.bmp");
	red_s.surfaces[1] = SDL_LoadBMP("Sprites/red/Down.bmp");
	red_s.surfaces[2] = SDL_LoadBMP("Sprites/red/Left.bmp");
	red_s.surfaces[3] = SDL_LoadBMP("Sprites/red/Right.bmp");

	yellow_s.surfaces[0] = SDL_LoadBMP("Sprites/yellow/Up.bmp");
	yellow_s.surfaces[1] = SDL_LoadBMP("Sprites/yellow/Down.bmp");
	yellow_s.surfaces[2] = SDL_LoadBMP("Sprites/yellow/Left.bmp");
	yellow_s.surfaces[3] = SDL_LoadBMP("Sprites/yellow/Right.bmp");
	
	fly_s.surfaces[0] = SDL_LoadBMP("Sprites/fly/Up.bmp");
	fly_s.surfaces[1] = SDL_LoadBMP("Sprites/fly/Down.bmp");
	fly_s.surfaces[2] = SDL_LoadBMP("Sprites/fly/Left.bmp");
	fly_s.surfaces[3] = SDL_LoadBMP("Sprites/fly/Right.bmp");

	wizard_s.surfaces[0] = SDL_LoadBMP("Sprites/wizard/Up.bmp");
	wizard_s.surfaces[1] = SDL_LoadBMP("Sprites/wizard/Down.bmp");
	wizard_s.surfaces[2] = SDL_LoadBMP("Sprites/wizard/Left.bmp");
	wizard_s.surfaces[3] = SDL_LoadBMP("Sprites/wizard/Right.bmp");

	//Cargo las texturas de la pantalla de inicio
	title_screen = SDL_LoadBMP("Sprites/title_screen.bmp");
	press_enter = SDL_LoadBMP("Sprites/press_enter.bmp");

	//Semilla para generacion de numeros random
	srand(time(NULL));
	//Colores base
	Blue = SDL_MapRGB(screen->format,20,63,211);
	Red = SDL_MapRGB(screen->format,255,0,0);
	Yellow = SDL_MapRGB(screen->format,255,255,0);
	Black = SDL_MapRGB(screen->format,0,0,0);
	White = SDL_MapRGB(screen->format,255,255,255);
	
	//Inicializa las propiedades de los portales
	Create_Portals();
	//Inicializa el valor de la puntuacion
	score = 0;

	//Tamaño, posicion,indice y Color base del jugador
	jugador.h = 56;
	jugador.w = 56;
	respawn_timer = 1920;
	jugador.type = "player";
	jugador.id = 0;

	//Inicializa todos los disparos a sus valores por defecto
	for (int i = 0; i < len(Shots); ++i)
	{
		Shots[i].type = "shot";
		Shots[i].id = i;
		Shots[i].from_id = 0;
		Shots[i].from_type = "NULL";
		Shots[i].w = 10;
		Shots[i].h = 10;
		Shots[i].Color = White;
	}
	//Pone todos los enemigos como inactivos
	for (int i = 0; i < len(Enemigos); ++i)
	{
		Enemigos[i].Alive = 0;
	}
}

//Dibuja una letra dada en una posicion dada
static void draw_letter(char letter,int x,int y){
	SDL_Rect Origen;
	letter -=47;
	Origen.x = 2*(letter)+24*(letter-1); //El punto desde el que se tomara la letra en x = pixeles de la imagen
	Origen.y = 2; //El punto desde el que se tomara la letra en y = pixeles de la imagen
	Origen.w = 24;
	Origen.h = 24;

	SDL_Rect Destino;
	Destino.x = x;
	Destino.y = y;
	SDL_BlitSurface(Font_map,&Origen,screen,&Destino);
}
//Dibuja una frase entera
static void draw_text(char text[],int x,int y){
	for (int i = 0; i < strlen(text); ++i)
	{
		draw_letter(text[i],x+i*24,y);
	}
}
//Dibuja cualquier objeto
static void draw_Object(Object *obj) {

	SDL_Rect src;

	src.x = obj->x;
	src.y = obj->y;
	src.w = obj->w;
	src.h = obj->h;

	int r = SDL_FillRect(screen, &src, obj->Color);
	
	if (r !=0){
		printf("No se pudo dibujar el muro");
	}
}

//Funciones Generales del motor
//Dibuja las entidades
static void draw_entity(Object *obj,int frames){
	SDL_SetColorKey(obj->surface,SDL_TRUE,Black);
	SDL_Rect src;
	if(obj->AnimFrame == (frames-1)){
		obj->AnimFrame = 0;
	}
	src.x = 54*obj->AnimFrame;
	src.y = 0;
	src.w = 54;
	src.h = 54;


	SDL_Rect dest;

	dest.x = obj->x;
	dest.y = obj->y;
	dest.w = obj->w;
	dest.h = obj->h;
	int r = SDL_BlitSurface(obj->surface,&src,screen,&dest);
	if (r !=0){
		printf("Fallo al dibujar %s\n",obj->type);
	}
}
//Detecta una colision entre dos objetos, en base a la red de movimiento del juego
static int detect_collision(Object *obj1,Object *obj2,int dis_x,int dis_y){

	int precision = 1;

	int interX = abs(dis_x) < (obj1->w/2 + obj2->w/2);
	int interY = abs(dis_y) < (obj1->h/2 + obj2->h/2);
	//Si se intersecan en x
	if(interX){
		//¿Choca por arriba?
		if(((obj1->y-(obj2->y+obj2->h)) < precision) & (dis_y > 0)){
			return 0;
		}
		//¿Choca por abajo?
		if(((obj2->y-(obj1->y+obj1->h)) < precision) & (dis_y < 0)){
			return 1;
		}
	}
	if(interY){
		//¿Choca por la derecha?
		if(((obj1->x-(obj2->x+obj2->w)) < precision) & (dis_x > 0)){
			return 2;
		}
		//¿Choca por la izquierda?
		if(((obj2->x-(obj1->x+obj1->w)) < precision) & (dis_x < 0)){
			return 3;
		}
	}

	//NO CHOCA
	return 4;
}

//Detecta colisiones entre el jugador y los enemigos
void player_enemy_collision(){
	for (int i = 0; i < len(Enemigos); ++i)
		{	
			if(Enemigos[i].Alive == 1){
				int ref_pos[2] = {jugador.x+jugador.w/2,jugador.y+jugador.h/2};
				int comp_pos[2] = {Enemigos[i].x+Enemigos[i].w/2,Enemigos[i].y+Enemigos[i].h/2};

				int dis_x = ref_pos[0]-comp_pos[0];
				int dis_y = ref_pos[1]-comp_pos[1];
				int distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
				if(distance < tile_size*tile_size*tile_size){
					if(detect_collision(&jugador,&Enemigos[i],dis_x,dis_y) != 4){
						//ACCIONES A REALIZAR SI UN JUGADOR CHOCA CON UNA ENTIDAD ENEMIGA
						if(!GOD_MODE){
							player_lifes--;
							respawn_timer=1920; //8 segundos
						}
					}
				}
			}
		}
}
//Detecta las colisiones de los disparos con las entidades y los muros del mapa
int shot_collision(Object *shot){
	//Comprueba colision con los muros
	static Object Vacio;
	Vacio.type = "Vacio";
	for (int i = 0; i < len(Muros); ++i)
	{
		int ref_pos[2] = {shot->x+shot->w/2,shot->y+shot->h/2};
		int comp_pos[2] = {Muros[i].x+Muros[i].w/2,Muros[i].y+Muros[i].h/2};

		int dis_x = ref_pos[0]-comp_pos[0];
		int dis_y = ref_pos[1]-comp_pos[1];
		int distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
		//Comprueba colisiones para objetos a menos de 64*sqrt(64)
		if(distance < tile_size*tile_size*tile_size){
			if(detect_collision(shot,&Muros[i],dis_x,dis_y) != 4){
				return 1;
			}
		}		
	}
	if(shot->from_type == "player"){
		for (int i = 0; i < len(Enemigos); ++i)
		{	
			if(Enemigos[i].Alive == 1){
				int ref_pos[2] = {shot->x+shot->w/2,shot->y+shot->h/2};
				int comp_pos[2] = {Enemigos[i].x+Enemigos[i].w/2,Enemigos[i].y+Enemigos[i].h/2};

				int dis_x = ref_pos[0]-comp_pos[0];
				int dis_y = ref_pos[1]-comp_pos[1];
				int distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
				if(distance < 64*tile_size*tile_size){
					if(detect_collision(shot,&Enemigos[i],dis_x,dis_y) != 4){
						//ACCIONES A REALIZAR SI UN DISPARO DEL JUGADOR CHOCA CON UNA ENTIDAD ENEMIGA
						Enemigos[i].Alive = 0;
						actual_enemy_number--;
						switch(Enemigos[i].enemy_type){
							case 1:
								killed_enemy[0]++;
								score +=100;
								break;
							case 2:
								killed_enemy[1]++;
								score +=200;
								break;
							case 3:
								killed_enemy[2]++;
								score +=500;
								break;
							case 4:
								killed_enemy[3]++;
								score +=1000;
								break;
							case 5:
								killed_enemy[4]++;
								score +=2500;
								break;
							}
						killed_enemy_cont++;
						return 1;
					}
				}
			}
		}
	}
	if(shot->from_type == "enemy"){
		int ref_pos[2] = {shot->x+shot->w/2,shot->y+shot->h/2};
		int comp_pos[2] = {jugador.x+jugador.w/2,jugador.y+jugador.h/2};

		int dis_x = ref_pos[0]-comp_pos[0];
		int dis_y = ref_pos[1]-comp_pos[1];
		int distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
		if(distance < 64*tile_size*tile_size){
			if(detect_collision(shot,&jugador,dis_x,dis_y) != 4){
				//Acciones si un enemigo le da al jugador
				if(!GOD_MODE){
					player_lifes--;
					respawn_timer = 1920;
				}
				return 1;
			}
		}
	}
	return 0;
}
//Detecta colisiones en las 4 direcciones posibles para las entidades enemigas y el jugador
static struct Coll_array detect_collisions(Object obj,Object Check2Objects[],int Check2Size){

	Coll_array temp_coll_s;

	temp_coll_s.array[0] = 0;
	temp_coll_s.array[1] = 0;
	temp_coll_s.array[2] = 0;
	temp_coll_s.array[3] = 0;

	for (int i = 0; i < Check2Size; i++)
	{

		int ref_pos[2] = {obj.x+obj.w/2,obj.y+obj.h/2};
		int comp_pos[2] = {Check2Objects[i].x+Check2Objects[i].w/2,Check2Objects[i].y+Check2Objects[i].h/2};

		int dis_x = ref_pos[0]-comp_pos[0];
		int dis_y = ref_pos[1]-comp_pos[1];
		int distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);

		if(distance < 10000){
			int direction = detect_collision(&obj,&Check2Objects[i],dis_x,dis_y);

			if(direction != 4){
				temp_coll_s.array[direction] = 1;
			}
		}

	}
	return temp_coll_s;
}

//Mueve cualquier estructura de tipo Object
static void move(Object *obj,int dx,int dy){
	obj->x +=dx;
	obj->y +=dy;
}
//Permite a los enemigos y al jugador disparar
static int Disparar(Object Disparador){	
	//Comprueba si el objeto ya disparó
	for (int i = 0; i < len(Shots); ++i)
	{
		if(Shots[i].from_type == Disparador.type){
			if(Shots[i].from_id == Disparador.id){
				return 0;
			}
		}
	}
	//Si no, asigna 1 disparo al Disparador
	for (int i = 0; i < len(Shots); ++i)
	{
		if(Shots[i].from_type == "NULL"){ //"NULL" significa que el disparo no esta vinculado a ningun objeto
			Shots[i].from_type = Disparador.type;
			Shots[i].from_id = Disparador.id;
			Shots[i].last_dir = Disparador.last_dir;
			if(Disparador.type == "enemy"){
				Shots[i].Color = Red;
			}else{
				Shots[i].Color = White;
			}
			if(Shots[i].last_dir == 0 || Shots[i].last_dir == 1){
				Shots[i].h = 20;
				Shots[i].w = 4;
			}
			if(Shots[i].last_dir == 2 || Shots[i].last_dir == 3){
				Shots[i].w = 20;
				Shots[i].h = 4;
			}
			Shots[i].x = Disparador.x+Disparador.w/2-Shots[i].w/2;
			Shots[i].y = Disparador.y+Disparador.h/2-Shots[i].h/2;
			switch(Shots[i].last_dir){
				case 0:
					Shots[i].y -=Disparador.h/2;
					break;
				case 1:
					Shots[i].y +=Disparador.h/2;
					break;
				case 2:
					Shots[i].x -=Disparador.w/2;
					break;
				case 3:
					Shots[i].x +=Disparador.w/2;
					break;
			}
			return 1;
		}
	}
}
//Actualiza la posicion y destruye los disparos si chocan
static void shots_update(){
	for (int i = 0; i < len(Shots); ++i)
	{
		if(Shots[i].from_type != "NULL"){
			int collide = shot_collision(&Shots[i]);
			if(collide){
				Shots[i].from_type = "NULL";
				continue;
			}
			switch(Shots[i].last_dir){
				case 0:
					move(&Shots[i],0,-4);
					break;
				case 1:
					move(&Shots[i],0,4);
					break;
				case 2:
					move(&Shots[i],-4,0);
					break;
				case 3:
					move(&Shots[i],4,0);
					break;
			}
		}
	}
}
//Funcion que aplica filtro de tv vieja al juego
int filter_alt = 0; //Variable para altenar filtros
static void apply_filters() {
	if(filter_alt == 2){
		SDL_Rect net;
	
		net.x = 0;
		net.y = 0;
		net.w = screen->w;
		net.h = 2;

		//draw the net
		int i,r;

		for(i = 0; i < screen->h/2; i++) {
			
			r = SDL_FillRect(screen, &net, Black);
		
			if (r != 0) { 
			
				printf("fill rectangle failed in func draw_net()");
			}

			net.y = net.y + 4;
		}
		filter_alt = 0;
	}else{
		filter_alt++;
	}
}

//Funciones del jugador
//Mueve al jugador basado en una entrada del teclado
static void move_player(int d) {

	Coll_array temp_coll_s = detect_collisions(jugador,Muros,len(Muros));

	int in_x_line = ((jugador.x + jugador.w/2 + tile_size/2) % tile_size) == 0;
	int in_y_line = ((jugador.y + jugador.h/2 + tile_size/2) % tile_size) == 0;

	//printf("UP:%i DOWN:%i LEFT:%i RIGHT:%i\n",temp_coll_s.array[0],temp_coll_s.array[1],temp_coll_s.array[2],temp_coll_s.array[3]);

	switch(d){
		case 0: //
			if(!temp_coll_s.array[0] & (in_x_line)){
				move(&jugador,0,-1);
				jugador.last_dir = 0;
			}
			break;
		case 1:
			if(!temp_coll_s.array[1] & (in_x_line)){
				move(&jugador,0,1);
				jugador.last_dir = 1;
			}
			break;
		case 2:
			if(!temp_coll_s.array[2] & (in_y_line)){
				move(&jugador,-1,0);
				jugador.last_dir = 2;
			}
			break;
		case 3:
			if(!temp_coll_s.array[3] & (in_y_line)){
				move(&jugador,1,0);
				jugador.last_dir = 3;
			}
			break;
	}
	if(jugador.x > SCREEN_WIDTH){
		jugador.x = -jugador.w;
	}
	if((jugador.x+jugador.w) < 0){
		jugador.x = SCREEN_WIDTH; 
	}
}

//Dibuja al jugador en pantalla
static void draw_player(int walking) {
	if((frame%24 == 0) & (walking)){
		jugador.AnimFrame++;
	}
	if(walking == 0){
		frames_quieto++;
	}else{
		frames_quieto = 0;
	}
	if(frames_quieto > 60){
		jugador.AnimFrame = 0;
		frames_quieto = 0;
	}
	jugador.surface = player_s.surfaces[jugador.last_dir];
	draw_entity(&jugador,5);
}

//Funciones del mapa
//Actualiza el Arreglo "Muros" con los muros del mapa (Map_Matrix) deseado
void Update_Walls(char Map_Matrix[FileY][FileX]){
	int cont = 0;
	for (int i = 0; i < FileY; i++)
	{
		for (int k = 0; k < FileX; k++)
		{
			char wall = Map_Matrix[i][k];
			if(wall != 'O'){
				Object temp_wall;
				temp_wall.type = "wall";
				temp_wall.Color = Blue;
				temp_wall.Alive = 1;
				if(wall == '|'){

					temp_wall.h = tile_size;
					temp_wall.w = tile_size/8;

					temp_wall.x = k*tile_size-temp_wall.w/2;
					temp_wall.y = ((i+1)/2)*tile_size;
				}
				if(wall == '-'){
					temp_wall.h = tile_size/8;
					temp_wall.w = tile_size;
					
					temp_wall.x = k*tile_size;
					temp_wall.y = (i/2)*tile_size - temp_wall.h/2 + tile_size;
				}
				if(wall == '/'){
					temp_wall.Alive = 0;
					temp_wall.h = tile_size;
					temp_wall.w = tile_size/8;
					temp_wall.x = k*tile_size-temp_wall.w/2;
					temp_wall.y = ((i+1)/2)*tile_size;
				}
				Muros[cont] = temp_wall;
				if(wall == '/'){
					PortalLWall = &Muros[cont];
					PortalLWall->x -=10;
				}
				cont++;

				//Duplicado simetrico
				temp_wall.x = screen->w - temp_wall.x - temp_wall.w;
				Muros[cont] = temp_wall;
				if(wall == '/'){
					PortalRWall = &Muros[cont];
					PortalRWall->x +=10;
				}
				cont++;
			}
		}
	}
}
//Dibuja los muros del mapa
static void draw_Map(){
	for (int i = 0; i < len(Muros); i++)
	{
		if(Muros[i].Alive)
			draw_Object(&Muros[i]);
	}
}
//Dibuja los disparos
static void draw_shots(){
	for (int i = 0; i < len(Shots); ++i)
	{
		if(Shots[i].from_type != "NULL"){
			draw_Object(&Shots[i]);
		}

	}
}

//Funciones de enemigos
//Hace aparecer a los enemigos
static void Spawn_Enemy(int type){
	static Object *enemy;
	for (int i = 0; i < len(Enemigos); ++i)
	{	
		//Busca un enemigo muerto
		if(Enemigos[i].Alive == 0){
			enemy = &Enemigos[i];
			break;
		}
	}
	switch(type){
		case 1:
			enemy->Color = Blue;
			break;
		case 2:
			enemy->Color = Yellow;
			break;
		case 3:
			enemy->Color = Red;
			break;
		case 4:
			enemy->Color = Red;
			break;
		case 5:
			enemy->Color = Black;
			break;
	}
	enemy->x = tile_size + tile_size*(rand()%11) + 4;
	enemy->y = tile_size + tile_size*(rand()%6) + 4;
	enemy->w = 56;
	enemy->h = 56;
	enemy->type = "enemy";
	enemy->AnimFrame = 0;
	enemy->last_dir = 4;
	enemy->enemy_cont = 0;
	enemy->id = actual_enemy_number;
	enemy->Alive = 1;
	enemy->enemy_type = type;
	enemy->invisible_timer = 0;
	actual_enemy_number++;
}
//Carga las texturas del enemigo
void Load_Enemy_surface(Object *enemy){
	switch(enemy->enemy_type){
		case 1:
			enemy->surface = blue_s.surfaces[enemy->last_dir];
			break;
		case 2:
			enemy->surface = yellow_s.surfaces[enemy->last_dir];
			break;
		case 3:
			enemy->surface = red_s.surfaces[enemy->last_dir];
			break;
		case 4:
			enemy->surface = fly_s.surfaces[enemy->last_dir];
			break;
		case 5:
			enemy->surface = wizard_s.surfaces[enemy->last_dir];
			break;
	}
}
//Dibuja a todos los enemigos vivos y no invisibles
static void Draw_Enemys(){
	for (int i = 0; i < len(Enemigos); i++){
		if((Enemigos[i].Alive == 1) & (Enemigos[i].invisible_timer <= 0)){
			if(Enemigos[i].invisible_timer < 0)
				Enemigos[i].invisible_timer = 0;
			Load_Enemy_surface(&Enemigos[i]);
			switch(Enemigos[i].enemy_type){
				case 1:
					draw_entity(&Enemigos[i],3);
					break;
				case 2:
					draw_entity(&Enemigos[i],4);
					break;
				case 3:
					draw_entity(&Enemigos[i],4);
					break;
				case 4:
					draw_entity(&Enemigos[i],4);
					break;
				case 5:
					draw_entity(&Enemigos[i],3);
					break;
			}
			if(frame % 24 == 0){
				Enemigos[i].AnimFrame++;
			}
		}
	}
}

//Controla el comportamiento de los enemigos
static void EnemyIA(){

	for (int i = 0; i < len(Enemigos); i++){
		if(Enemigos[i].Alive  == 0){
			continue;
		}

		//Controla la capacidad de algunos enemigos de volversen invisibles al jugador
		if((Enemigos[i].enemy_type == 2) || (Enemigos[i].enemy_type == 3) || (Enemigos[i].enemy_type == 5)){
			int inv = 0;
			if(Enemigos[i].enemy_type == 5){
				inv = rand()%1000;
				if((Enemigos[i].invisible_timer == 0) & (inv == 1000/2)){
					Enemigos[i].invisible_timer = 50;
					Enemigos[i].x = tile_size + tile_size*(rand()%11) + 4;
					Enemigos[i].y = tile_size + tile_size*(rand()%6) + 4;
					Enemigos[i].enemy_cont = 0;
					Enemigos[i].last_dir = 4;
				}
				if(Enemigos[i].invisible_timer > 0){
					Enemigos[i].invisible_timer--;
				}
			}else{
				inv = rand()%1000;
				if((Enemigos[i].invisible_timer == 0) & (inv == 1000/2)){
					Enemigos[i].invisible_timer = 150;
				}
				if(Enemigos[i].invisible_timer > 0){
					Enemigos[i].invisible_timer--;
				}
			}
			
		}
		//Probabilidad de que dispare el enemigo
		int r = rand()%(1000);
		if((r == 1000/2) & (Enemigos[i].last_dir != 4) & (Enemigos[i].enemy_type != 4)){
			Disparar(Enemigos[i]);
		}
		//Collisones actuales del enemigo
		Coll_array temp_coll_s = detect_collisions(Enemigos[i],Muros,len(Muros));

		int coll[4];

		coll[0] = temp_coll_s.array[0];
		coll[1] = temp_coll_s.array[1];
		coll[2] = temp_coll_s.array[2];
		coll[3] = temp_coll_s.array[3];


		int in_x_line = ((Enemigos[i].x + Enemigos[i].w/2 + tile_size/2) % tile_size) == 0;
		int in_y_line = ((Enemigos[i].y + Enemigos[i].h/2 + tile_size/2) % tile_size) == 0;

		//Esta en interseccion?
		int inter = !coll[0]*!coll[2]+!coll[2]*!coll[1]+!coll[1]*!coll[3]+!coll[3]*!coll[0];
		//Esta en una intersección
		if(((inter != 0) || (Enemigos[i].last_dir == 4))&(Enemigos[i].enemy_cont == 0)){
			int count = 0;
			char valid_dir[4];
			if((coll[0] == 0)  & (Enemigos[i].last_dir != 1)){
				valid_dir[count] = 'u';
				count++;
			}
			if((coll[1] == 0) & (Enemigos[i].last_dir != 0)){
				valid_dir[count] = 'd';
				count++;
			}
			if((coll[2] == 0) & (Enemigos[i].last_dir != 3)){
				valid_dir[count] = 'l';
				count++;
			}
			if((coll[3] == 0) & (Enemigos[i].last_dir != 2)){
				valid_dir[count] = 'r';
				count++;
			}

			int dir = rand()%count;

			switch(valid_dir[dir]){
				case 'u':
					if(in_x_line){
						move(&Enemigos[i],0,-1);
						Enemigos[i].last_dir = 0;
					}
					break;
				case 'd':
					if(in_x_line){
						move(&Enemigos[i],0,1);
						Enemigos[i].last_dir = 1;
					}
					break;
				case 'l':
					if(in_y_line){
						move(&Enemigos[i],-1,0);
						Enemigos[i].last_dir = 2;
					}
					break;
				case 'r':
					if(in_y_line){
						move(&Enemigos[i],1,0);
						Enemigos[i].last_dir = 3;
					}
					break;
			}
			Enemigos[i].enemy_cont++;
		}else{
			//Pasillo vertical
			if((Enemigos[i].last_dir == 0) & !coll[0]){
				move(&Enemigos[i],0,-1);
				Enemigos[i].enemy_cont++;
			}
			if((Enemigos[i].last_dir == 1) & !coll[1]){
				move(&Enemigos[i],0,1);
				Enemigos[i].enemy_cont++;
			}
			//Pasillo horizontal
			if((Enemigos[i].last_dir == 2) & !coll[2]){
				move(&Enemigos[i],-1,0);
				Enemigos[i].enemy_cont++;
			}
			if((Enemigos[i].last_dir == 3) & !coll[3]){
				move(&Enemigos[i],1,0);
				Enemigos[i].enemy_cont++;
			}
		}
		if(Enemigos[i].enemy_cont == tile_size){
			Enemigos[i].enemy_cont = 0;
		}
	}
}

//Inicializa las propiedades de los portales del mapa
void Create_Portals(){
	PortalL.h = tile_size - 10;
	PortalL.w = 4;
	PortalL.Color = Red;
	PortalL.x = tile_size - PortalL.w;
	PortalL.y = tile_size*3 + 5;

	PortalR.h = tile_size - 10;
	PortalR.w = 4;
	PortalR.Color = Red;
	PortalR.x = screen->w - tile_size;
	PortalR.y = tile_size*3 + 5;
}

int Portal_Cooldown = 0; //Variable que controla el tiempo que pasan los portales inactivos una vez son usados
int Update_Portals(){
	if(Portal_Cooldown == 0){
		for (int i = 0; i < len(Enemigos); ++i)
		{
			if(Enemigos[i].Alive){
				//Para el portal derecho
				int ref_pos[2] = {PortalR.x+PortalR.w/2,PortalR.y+PortalR.h/2};
				int comp_pos[2] = {Enemigos[i].x+Enemigos[i].w/2,Enemigos[i].y+Enemigos[i].h/2};

				int dis_x = ref_pos[0]-comp_pos[0];
				int dis_y = ref_pos[1]-comp_pos[1];
				int distance = 0;

				dis_x = ref_pos[0]-comp_pos[0];
				dis_y = ref_pos[1]-comp_pos[1];
				distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
				if(distance < 64*tile_size*tile_size){
					if(detect_collision(&PortalR,&Enemigos[i],dis_x,dis_y) != 4){
						//ACCIONES A REALIZAR SI UN PORTAL CHOCA CON UNA ENTIDAD ENEMIGA
						if(Enemigos[i].enemy_type == 4){
							killed_enemy[3]++;
							Enemigos[i].Alive = 0;
						}
						Enemigos[i].x -=tile_size*10;
						Portal_Cooldown = 2400;
						PortalRWall->x -=10;
						PortalLWall->x +=10;
						return 0;
					}
				}else{
					//Para el portal izquierdo
					ref_pos[0] = PortalL.x+PortalL.w/2;
					ref_pos[1] = PortalL.y+PortalL.h/2;

					dis_x = ref_pos[0]-comp_pos[0];
					dis_y = ref_pos[1]-comp_pos[1];
					distance = abs(dis_x*dis_x) + abs(dis_y*dis_y);
					if(distance < 64*tile_size*tile_size){
						if(detect_collision(&PortalL,&Enemigos[i],dis_x,dis_y) != 4){
							//ACCIONES A REALIZAR SI UN PORTAL CHOCA CON UNA ENTIDAD ENEMIGA
							if(Enemigos[i].enemy_type == 4){
								killed_enemy[3]++;
								Enemigos[i].Alive = 0;
							}
							Enemigos[i].x +=tile_size*10;
							Portal_Cooldown = 2400;
							PortalRWall->x -=10;
							PortalLWall->x +=10;
							return 0;
						}
					}
				}
			}
		}
		int ref_pos[2] = {PortalR.x+PortalR.w/2,PortalR.y+PortalR.h/2};
		int comp_pos[2] = {jugador.x+jugador.w/2,jugador.y+jugador.h/2};

		int dis_x = ref_pos[0]-comp_pos[0];
		int dis_y = ref_pos[1]-comp_pos[1];
		if(detect_collision(&PortalR,&jugador,dis_x,dis_y) != 4){
			//ACCIONES A REALIZAR SI UN PORTAL CHOCA CON EL JUGADOR
			jugador.x -=tile_size*10;
			Portal_Cooldown = 2400;
			PortalRWall->x -=10;
			PortalLWall->x +=10;
			return 0;
		}else{
			ref_pos[0] = PortalL.x+PortalL.w/2;
			ref_pos[1] = PortalL.y+PortalL.h/2;

			dis_x = ref_pos[0]-comp_pos[0];
			dis_y = ref_pos[1]-comp_pos[1];

			if(detect_collision(&PortalL,&jugador,dis_x,dis_y) != 4){
				//ACCIONES A REALIZAR SI UN PORTAL CHOCA CON EL JUGADOR
				jugador.x +=tile_size*10;
				Portal_Cooldown = 2400;
				PortalRWall->x -=10;
				PortalLWall->x +=10;
				return 0;
			}

		}
	}else{
		Portal_Cooldown--;
		if(Portal_Cooldown % 2 == 0){
			PortalL.Color = Yellow;
			PortalR.Color = Yellow;
		}else{
			PortalL.Color = Red;
			PortalR.Color = Red;
		}
		if(Portal_Cooldown == 0){
			PortalL.Color = Red;
			PortalR.Color = Red;
			PortalRWall->x +=10;
			PortalLWall->x -=10;
		}
	}
	
}

//Dibuja los portales en pantalla
void draw_Portals(){
	draw_Object(&PortalL);
	draw_Object(&PortalR);
}


//Funciones de cambio de estado
//Resetea las propiedades mas importantes del arreglo Muros antes de cargar un nuevo mapa
void clear_map_data(){
	for (int i = 0; i < len(Muros); ++i)
	{
		Muros[i].x = 0;
		Muros[i].y = 0;

		Muros[i].w = 0;
		Muros[i].h = 0;
	}
}
//Carga un mapa seleccionado al arreglo Muros
void Level_Select(int map_id){
	clear_map_data();
	switch(map_id){
		case 1:
			Update_Walls(Map1_Matrix);
			break;
		case 2:
			Update_Walls(Map2_Matrix);
			break;
		case 3:
			Update_Walls(Map3_Matrix);
			break;
	}
}
//Cambia el estado del juego y resetea los parametros importantes a dicho estado
void change_to_state(int state_to){
	state = state_to;
	switch(state_to){
		case 0:
			actual_enemy_number = 0;
			dificultad = 0;
			killed_enemy_cont = 0;
			jugador.AnimFrame = 0;
			for (int i = 0; i < len(Enemigos); ++i)
			{
				Enemigos[i].Alive = 0;
				Enemigos[i].enemy_type = 1;
			}
			memset(type,0,sizeof(type));
			memset(killed_enemy,0,sizeof(killed_enemy));
			break;
		case 1:
			GOD_MODE = 0;
			Level_Select(1);
			respawn_timer = 1920;
			dificultad = 1;
			Level = 1;
			killed_enemy_cont = 0;
			score=0;
			player_lifes = 4;
			frames_quieto = 0;
			jugador.last_dir = 2;
			killed_enemy_cont = 0;
			jugador.x = 708;
			jugador.y = 452;
			Portal_Cooldown = 0;
			break;
		case 2:
			break;

	}
	SDL_Delay(500);
}

int WIN = 0; //Para distingir si el jugador gano o perdio el juego
//Funciones de interfaz
//Dibuja la interfaz dependiendo del estado del juego
void draw_UI(){
	if(state == 1){
		if(GOD_MODE){
			Object god_indicator;
			god_indicator.x = 10;
			god_indicator.y = 10;
			god_indicator.w = 20;
			god_indicator.h = 20;
			god_indicator.Color = Red;
			draw_Object(&god_indicator);
		}
		Radar();
		Object ScoreBox1,ScoreBox2;

		ScoreBox1.x = screen->w - 224;
		ScoreBox1.y = screen->h - 112;
		ScoreBox1.w = 224;
		ScoreBox1.h = 112;
		ScoreBox1.Color = Yellow;

		ScoreBox2.x = screen->w - 196;
		ScoreBox2.y = screen->h - 84;
		ScoreBox2.w = 168;
		ScoreBox2.h = 56;
		ScoreBox2.Color = Black;

		draw_Object(&ScoreBox1);
		draw_Object(&ScoreBox2);

		char SCORE_string[6];
		sprintf(SCORE_string,"%i",score);
		draw_text(SCORE_string,screen->w-185,screen->h-66);


		static Object Life_Entity;
		Life_Entity.AnimFrame = 0;
		Life_Entity.w = jugador.w;
		Life_Entity.h = jugador.h;
		Life_Entity.surface = Player_Life_surf;
		if(respawn_timer == 0){
			if(player_lifes > 1){
			Life_Entity.x = screen->w - jugador.w - tile_size - 4;
			Life_Entity.y = screen->h - jugador.h - 131;
			draw_entity(&Life_Entity,1);
			}
			for (int i = 0; i < (player_lifes-2); ++i)
			{
				Life_Entity.x = screen->w - jugador.w - 4;
				Life_Entity.y = screen->h - jugador.h*(i+1) - 131;
				draw_entity(&Life_Entity,1);
			}
		}else{
			for (int i = 0; i < (player_lifes-1); ++i)
			{
				Life_Entity.x = screen->w - jugador.w - 4;
				Life_Entity.y = screen->h - jugador.h*(i+1) - 131;
				draw_entity(&Life_Entity,1);
			}
		}
		

		char killed_enemy_cont_text[100];
		sprintf(killed_enemy_cont_text,"KILLS = %i",killed_enemy_cont);
		draw_text(killed_enemy_cont_text,20,screen->h-24*4);
		

		char Level_Text[10];
		sprintf(Level_Text,"DUNGEON %i",Level);
		draw_text(Level_Text,screen->w/2 - 24*4,24);
		if(respawn_timer/240 >= 1){
			char respawn_timer_text[3];
			sprintf(respawn_timer_text,"%i",respawn_timer/240);
			draw_text(respawn_timer_text,screen->w-tile_size*2-34,470);
		}
	}
	if(state == 2){
		if(WIN){
			draw_text("YOU WIN",screen->w/2-84,200);
		}else{
			draw_text("GAME OVER",screen->w/2-108,200);
		}
		char score_text[10];
		sprintf(score_text,"SCORE = %i",score);
		draw_text(score_text,screen->w/2-12*strlen(score_text),250);
		draw_text("PRESS ESCAPE TO RESTART",screen->w/2-252,300);
		
	}
}

//Maneja la aparicion de enemigos durante el juego
void Game_Manager(){
	
	//No se ha eliminado ningun enemigo 1 y no hay enemigos 1 => Crea 6
	if(type[0] == 0){
				Spawn_Enemy(1);
				Spawn_Enemy(1);
				Spawn_Enemy(1);
				Spawn_Enemy(1);
				Spawn_Enemy(1);
				Spawn_Enemy(1);
				type[0]+=6;
	}
	switch(Level){
		case 1:
			if((type[1] < 1) & (killed_enemy[1] < 1)){
				if((killed_enemy[0] == 6) & (type[1] == 0)){
					Spawn_Enemy(2);
					type[1]++;
				}
				
			}
			if((killed_enemy[1] == 1) & (type[2] == 0)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[2] == 1) & (type[3] == 0)){
				Spawn_Enemy(4);
				dificultad=6;
				type[3]++;
			}
			if(killed_enemy[3] == 1){
				dificultad=2;
				Level = 2;
				memset(type,0,sizeof(type));
				memset(killed_enemy,0,sizeof(killed_enemy));
				respawn_timer = 1920;
			}
			break;
		case 2:
			if((type[1] < 2) & (killed_enemy[1] < 2)){
				if((killed_enemy[0] == 5) & (type[1] == 0)){
					Spawn_Enemy(2);
					type[1]++;
				}
				if((killed_enemy[0] == 6) & (type[1] == 1)){
					Spawn_Enemy(2);
					type[1]++;
				}
				
			}
			if((killed_enemy[1] == 1) & (type[2] == 0)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[1] == 2) & (type[2] == 1)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[2] == 2) & (type[3] == 0)){
				Spawn_Enemy(4);
				dificultad=6;
				type[3]++;
			}
			if(killed_enemy[3] == 1){
				dificultad=3;
				Level = 3;
				memset(type,0,sizeof(type));
				memset(killed_enemy,0,sizeof(killed_enemy));
				respawn_timer = 1920;
			}
			break;
		case 3:
			if((type[1] < 3) & (killed_enemy[1] < 3)){
				if((killed_enemy[0] == 4) & (type[1] == 0)){
					Spawn_Enemy(2);
					type[1]++;
				}
				if((killed_enemy[0] == 5) & (type[1] == 1)){
					Spawn_Enemy(2);
					type[1]++;
				}
				if((killed_enemy[0] == 6) & (type[1] == 2)){
					Spawn_Enemy(2);
					type[1]++;
				}
			}
			if((killed_enemy[1] == 1) & (type[2] == 0)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[1] == 2) & (type[2] == 1)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[1] == 3) & (type[2] == 2)){
				Spawn_Enemy(3);
				type[2]++;
			}
			if((killed_enemy[2] == 3) & (type[3] == 0)){
				Spawn_Enemy(4);
				dificultad=6;
				type[3]++;
			}
			if((killed_enemy[3] == 1) & (type[4] == 0)){
				Spawn_Enemy(5);
				type[4]++;
				dificultad=6;
			}
			if(killed_enemy[4] == 1){
				state = 2;
				WIN = 1;
				memset(type,0,sizeof(type));
				memset(killed_enemy,0,sizeof(killed_enemy));
			}
			break;
	}	
}
//Muestra a todos los enemigos menos el WIZARD OF WOR en un radar en la parte baja de la ventana del juego
void Radar(){
	Object RadarExternal,RadarInternal;

	RadarExternal.w = 290;
	RadarExternal.h = 160;
	RadarExternal.x = screen->w/2 - RadarExternal.w/2;
	RadarExternal.y = screen->h - RadarExternal.h - 20;
	RadarExternal.Color = Blue;

	RadarInternal.w = 286;
	RadarInternal.h = 156;
	RadarInternal.x = screen->w/2 - RadarInternal.w/2;
	RadarInternal.y = screen->h - RadarInternal.h - (20+(RadarExternal.w-RadarInternal.w)/2);
	RadarInternal.Color = Black;

	draw_Object(&RadarExternal);
	draw_Object(&RadarInternal);

	Object point; //Punto en el radar
	point.w = 14;
	point.h = 14;

	for (int i = 0; i < len(Enemigos); ++i)
	{
		if(Enemigos[i].Alive){
			point.x = RadarInternal.x;
			point.y = RadarInternal.y;
			point.x +=26*((Enemigos[i].x+Enemigos[i].w/2)/64 -1)+7;
			point.y +=26*((Enemigos[i].y+Enemigos[i].h/2)/64 -1)+7;
			point.Color = Enemigos[i].Color;
			draw_Object(&point);
		}
	}
}

//Funciones para la ejecucion del juego
int main (int argc, char *args[]) {
	//SDL Window setup
	if (init(SCREEN_WIDTH, SCREEN_HEIGHT, argc, args) == 1) {
		return 0;
	}
	
	SDL_SetColorKey(press_enter,SDL_TRUE,Black);

	SDL_GetWindowSize(window, &width, &height);
	
	int sleep = 0;
	int quit = 0;
	int r = 0;
	Uint32 next_game_tick = SDL_GetTicks();
	init_game();

	int press_enter_timer = 480; //Tiempo que tarda un ciclo de la pantalla de inicio
	//render loop
	while(quit == 0) {
		//check for new events every frame
		SDL_PumpEvents();

		const Uint8 *keystate = SDL_GetKeyboardState(NULL);

		//draw background
		SDL_RenderClear(renderer);
		SDL_FillRect(screen, NULL,Black);
		draw_UI();
		if(state == 0){
			
			//Salir del juego
			if (keystate[SDL_SCANCODE_ESCAPE])
				quit = 1;

			//Dibuja la pantalla de inicio
			
			SDL_Rect src;
			src.x = 0;
			src.y = 0;
			src.w = SCREEN_WIDTH;
			src.h = SCREEN_HEIGHT;

			SDL_Rect dest;

			dest.x = 0;
			dest.y = 0;
			dest.w = SCREEN_WIDTH;
			dest.h = SCREEN_HEIGHT;
			if(press_enter_timer < 240){
				SDL_BlitSurface(press_enter,&src,screen,&dest);
			}else{
				SDL_BlitSurface(title_screen,&src,screen,&dest);
			}
			if(press_enter_timer == 0){
				press_enter_timer = 480;
			}

			//Empezar juego => Ir a estado 1
			if(keystate[SDL_SCANCODE_RETURN]){
				change_to_state(1);
			}
			press_enter_timer--;
		}
		
		if(state == 1){
			//Comprueba si se quiere salir del estado
			if (keystate[SDL_SCANCODE_ESCAPE]){
				change_to_state(0);
			}
			else{ //Si no sigue el juego
				Update_Portals();
				if(keystate[SDL_SCANCODE_SPACE] & (respawn_timer != 0)){
					respawn_timer = 65;
				}

				if(keystate[SDL_SCANCODE_G]){
					GOD_MODE = !GOD_MODE;
				}

				if(player_lifes <= 0){
					change_to_state(2);
					continue;
				}
				//Dibuja el mapa
				
				draw_Map();
				
				respawn(respawn_timer);
				//Control de la IA Enemiga
				if(frame%(max_dificultad+1-dificultad) == 0){
					EnemyIA();
				}
				//Control de jugador
				int walking = 0;
				if((frame%2 == 0) & (respawn_timer == 0)){
					////Mover
					if (keystate[SDL_SCANCODE_UP]) {
						move_player(0);
						walking = 1;
					}

					if (keystate[SDL_SCANCODE_DOWN]) {
						move_player(1);
						walking = 1;
					}

					if(keystate[SDL_SCANCODE_LEFT]){
						move_player(2);
						walking = 1;
					}

					if(keystate[SDL_SCANCODE_RIGHT]){
						move_player(3);
						walking = 1;
					}
					////Disparar
					if(keystate[SDL_SCANCODE_RETURN]){
						Disparar(jugador);
						//SDL_Delay(100);
					}
					player_enemy_collision();
					
				}
				
				if(frame%2 == 0){
					//Actualiza los disparos
					shots_update();
				}
				
				//Dibuja jugador
				draw_player(walking);
				//Dibuja a los enemigos
				Draw_Enemys();
				//Dibuja los disparos
				draw_shots();

				draw_Portals();

				
				//Controlla el avance del juego
				Game_Manager();
				
			}
		}
		if(state == 2){
			if(keystate[SDL_SCANCODE_ESCAPE]){
				change_to_state(0);
			}
		}


		apply_filters();
		SDL_UpdateTexture(screen_texture, NULL, screen->pixels, screen->w * sizeof (Uint32));
		SDL_RenderCopy(renderer, screen_texture, NULL, NULL);

		
		//draw to the display
		SDL_RenderPresent(renderer);
			
		//time it takes to render  frame in milliseconds
		next_game_tick += 1000 /240;
		frame++;
		sleep = next_game_tick - SDL_GetTicks();
	
		if( sleep >= 0 ) {
            				
			SDL_Delay(sleep);
		}
	}

	//free loaded images
	SDL_FreeSurface(screen);
	SDL_FreeSurface(Font_map);
	SDL_FreeSurface(title_screen);


	//free renderer and all textures used with it
	SDL_DestroyRenderer(renderer);
	
	//Destroy window 
	SDL_DestroyWindow(window);

	//Quit SDL subsystems 
	SDL_Quit(); 
	 
	return 0;
}

int init(int width, int height, int argc, char *args[]) {

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {

		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		
		return 1;
	} 
	
	int i;
	
	for (i = 0; i < argc; i++) {
		
		//Create window	
		if(strcmp(args[i], "-f")) {
			
			SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &window, &renderer);
		
		} else {
		
			SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer);
		}
	}

	if (window == NULL) { 
		
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		
		return 1;
	}

	//create the screen sruface where all the elemnts will be drawn onto (ball, paddles, net etc)
	screen = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
	
	if (screen == NULL) {
		
		printf("Could not create the screen surfce! SDL_Error: %s\n", SDL_GetError());

		return 1;
	}

	//create the screen texture to render the screen surface to the actual display
	screen_texture = SDL_CreateTextureFromSurface(renderer, screen);

	if (screen_texture == NULL) {
		
		printf("Could not create the screen_texture! SDL_Error: %s\n", SDL_GetError());

		return 1;
	}
	return 0;
}
