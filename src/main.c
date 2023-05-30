#define GLUT_DISABLE_ATEXIT_HACK
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDl2/SDL_opengl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <stdio.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <load.h>
#include <info.h>
#include <draw.h>
#include <transform.h>
#include <float.h>

#define WIDTH 1024
#define HEIGHT 768
#define FOV 90.0
#define NEAR_CLIP 0.1
#define FAR_CLIP 100.0
#define MOVE_SPEED 0.03
#define MOUSE_SENSITIVITY 0.2

#define MAZE_WIDTH 20 //X
#define MAZE_HEIGHT 20 //Z
#define MAZE_DEPTH 1 //Y
#define WALL 1
#define ROUTE 0
#define PLAYER_RADIUS 0.2f

#define CELL_SIZE 1.0f

typedef struct {
    float x;
    float y;
    float z;
} vec3;

typedef struct {
    vec3 position;
    float yaw;
    float pitch;
} camera;

camera cam = {
    .position = {0, 0, 0},
    .yaw = 0.0,
    .pitch = 0.0
};

typedef struct {
    int x;
    int y;
} Cell;

bool keys[256] = { false };


int numWalls = 0;

int wallPositions[MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH*2][2][2][3];
int wallCount = 0;

int maze[MAZE_HEIGHT][MAZE_WIDTH];

	typedef struct {
    float direction[3];  // Direction of sunlight
    float intensity;     // Intensity of sunlight
} Sunlight;

Sunlight sunlight;

typedef struct {
    Model model;
    float material_ambient[4];
    int texture;
    struct Vertex position;
}Object;


void changeLightintensity(float amount) {
    
        sunlight.intensity += amount;
		if (sunlight.intensity < 0.0f) {
			sunlight.intensity = 0.0f;			
		}
		else if (sunlight.intensity > 1.0f){
			sunlight.intensity = 1.0f;
		}
 
	GLfloat sun_intensity[] = { sunlight.intensity, sunlight.intensity, sunlight.intensity, 1.0 };
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sun_intensity);
	    printf("Sunlight intensity:\n");
        printf("%.2f\n", sunlight.intensity);
    }

int isHelpOn = 0;
int firstRow, firstCol;
int secondRow, secondCol;	
int exitRow, exitCol;
int haveKey = 0;
int exitActionsPerformed = 0;
bool doorOpen = false;
float doorRotation = 0.0f;

GLfloat light_position[] = {0, 30, 30};//, 0};
GLfloat light_ambient[] = { 0.8, 0.8, 0.8, 0.8};
GLfloat light_diffuse[] = {0.3, 0.3, 0.3, 0.9};
GLfloat light_specular[] = {1.0, 1.0, 1.0, 1.0};

void init() {
    // Enable lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void handle_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                exit(0);
                break;
            case SDL_KEYDOWN:
                keys[event.key.keysym.scancode] = true;
                break;
            case SDL_KEYUP:
                keys[event.key.keysym.scancode] = false;
				if (event.key.keysym.sym == SDLK_ESCAPE)
					exit(0);
                else if (event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_PLUS) {
                    changeLightintensity(0.1f);
                } else if (event.key.keysym.sym == SDLK_KP_MINUS || event.key.keysym.sym == SDLK_MINUS) {
                    changeLightintensity(-0.1f);
                }
				if (event.key.keysym.sym == SDLK_F1){
						if(isHelpOn == 1)
							{
							isHelpOn=0;
							}
						else isHelpOn = 1;
						}
				
                break;
            case SDL_MOUSEMOTION:
                cam.yaw += event.motion.xrel * MOUSE_SENSITIVITY;
                cam.pitch -= event.motion.yrel * MOUSE_SENSITIVITY;
                if (cam.pitch < -89.0) cam.pitch = -89.0;
                if (cam.pitch > 89.0) cam.pitch = 89.0;
                break;
        }
    }
}

	
GLuint loadTexture(const char* filePath) {
	
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	
    SDL_Surface* textureSurface = SDL_LoadBMP(filePath);
    if (!textureSurface) {
        printf("Error loading texture: %s\n", SDL_GetError());
        return 0;
    }
	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureSurface->w, textureSurface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, textureSurface->pixels);
    SDL_FreeSurface(textureSurface);
	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
    return textureID;
}

float calculateDistance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

int isPlayerCloseToExit() {
    // Calculate the distance between the player and the exit based on their coordinates
    // Replace this calculation with your own logic based on your game's requirements
    float distance = calculateDistance(cam.position.x, cam.position.z, exitRow, exitCol);

    // Adjust the threshold value as needed
    float threshold = 2.0f;

    // Return 1 if the player is close to the exit, otherwise return 0
    return (distance <= threshold);
}



void drawMaze() {
GLuint faltextura = loadTexture("assets/brickwall.bmp");
GLuint talajtextura = loadTexture("assets/ground.bmp");
glEnable(GL_TEXTURE_2D);
glEnable(GL_LIGHTING);
glBindTexture(GL_TEXTURE_2D,faltextura);

    glBegin(GL_QUADS);
    for (int x = 0; x < MAZE_WIDTH; x++) {
            for (int z = 0; z < MAZE_HEIGHT; z++) {
                if (maze[x][z] == WALL) {
                    
					float xmin = x * CELL_SIZE;
                    float xmax = (x + 1) * CELL_SIZE;
                    float zmin = z * CELL_SIZE;
                    float zmax = (z + 1) * CELL_SIZE;
					
					// Draw front face					
					glNormal3f(0.0f, 0.0f, -1.0f);
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmin, 0, zmin);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmax, 0, zmin);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmax, MAZE_DEPTH, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmin);
                    glEnd();
					
                    // Draw back face				
					glNormal3f(0.0f, 0.0f, 1.0f);
                    glBegin(GL_QUADS);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmin, 0, zmax);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmax, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmax, 0, zmax);
                    glEnd();
					
                    // Draw left face				
					glNormal3f(-1.0f, 0.0f, 0.0f);
                    glBegin(GL_QUADS);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmin, 0, zmin);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmin, 0, zmax);
                    glEnd();
					
					
                    // Draw right face				
					glNormal3f(1.0f, 0.0f, 0.0f);				
                    glBegin(GL_QUADS);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmax, 0, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmax, 0, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmax, MAZE_DEPTH, zmax);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmax, MAZE_DEPTH, zmin);
                    glEnd();
					
					// Draw top face
					glNormal3f(0.0f, 1.0f, 0.0f);					
					glBegin(GL_QUADS);
					glVertex3f(xmin, MAZE_DEPTH, zmin);
					glVertex3f(xmax, MAZE_DEPTH, zmin);
					glVertex3f(xmax, MAZE_DEPTH, zmax);
					glVertex3f(xmin, MAZE_DEPTH, zmax);
					
					// Draw bottom face
					glNormal3f(0.0f, -1.0f, 0.0f);
					glBegin(GL_QUADS);
					glVertex3f(xmin,0, zmin);
					glVertex3f(xmax,0, zmin);
					glVertex3f(xmax,0, zmax);
					glVertex3f(xmin,0, zmax);
				}
			}
		}
	
	
for (int i = 0; i <= MAZE_WIDTH-1; i++) {
    //north
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(i * CELL_SIZE, 0.0f, MAZE_HEIGHT);
    glTexCoord2f(1.0f, 0.0f); glVertex3f((i+1) * CELL_SIZE, 0.0f, MAZE_HEIGHT);
    glTexCoord2f(1.0f, 1.0f); glVertex3f((i+1) * CELL_SIZE, MAZE_DEPTH, MAZE_HEIGHT);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(i * CELL_SIZE, MAZE_DEPTH, MAZE_HEIGHT);
}

for (int i = 0; i < MAZE_WIDTH; i++) {
    //south
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(i * CELL_SIZE, 0.0, 0.0);
    glTexCoord2f(1.0f, 0.0f); glVertex3f((i + 1) * CELL_SIZE, 0.0, 0.0);
    glTexCoord2f(1.0f, 1.0f); glVertex3f((i + 1) * CELL_SIZE, MAZE_DEPTH, 0.0);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(i * CELL_SIZE, MAZE_DEPTH, 0.0);
}

for (int i = 0; i < MAZE_HEIGHT; i++) {
    //east
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0, 0.0, i*CELL_SIZE);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0, MAZE_DEPTH, i*CELL_SIZE);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0, MAZE_DEPTH, (i+1)*CELL_SIZE);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0, 0.0, (i+1)*CELL_SIZE);
}

for (int i = 0; i < exitCol-1; i++) {
    //west
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, i*CELL_SIZE);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, i*CELL_SIZE);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, (i+1)*CELL_SIZE);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, (i+1)*CELL_SIZE);
}
for (int i = exitCol; i < MAZE_HEIGHT; i++) {
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, i*CELL_SIZE);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, i*CELL_SIZE);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, (i+1)*CELL_SIZE);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, (i+1)*CELL_SIZE);
}
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol-0.75);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol-0.75);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol-1.0);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol-1.0); 
	
	glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol-0.25);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol-0.25); 

	//Cup room
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-0.75);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-0.75);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol-0.75);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol-0.75); 
	
	glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, exitCol-0.25);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, exitCol-0.25);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-0.25);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-0.25); 
	
	glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-0.75);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-0.75);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-1.0);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-1.0); 
	
	glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-0.25);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-0.25);

	for (int i = 1; i < 3; i++) {
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-1+((i+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-1+((i+1)*CELL_SIZE));
		
	glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-4+(i*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-4+(i*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0, MAZE_DEPTH, exitCol-4+((i+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0, 0.0, exitCol-4+((i+1)*CELL_SIZE));
	
}

	for (int i = 0; i < 5; i++) {
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0+(i+1*CELL_SIZE), 0.0, exitCol-3);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0+(i+1*CELL_SIZE), MAZE_DEPTH, exitCol-3);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0+(i*CELL_SIZE), MAZE_DEPTH, exitCol-3);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0+(i*CELL_SIZE), 0.0, exitCol-3);
	
	glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0+(i+1*CELL_SIZE), 0.0, exitCol+2);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0+(i+1*CELL_SIZE), MAZE_DEPTH, exitCol+2);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+1.0+(i*CELL_SIZE), MAZE_DEPTH, exitCol+2);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+1.0+(i*CELL_SIZE), 0.0, exitCol+2);
	
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+6.0, 0.0, exitCol-3+(i*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+6.0, MAZE_DEPTH, exitCol-3+(i*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+6.0, MAZE_DEPTH, exitCol-3+((i+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+6.0, 0.0, exitCol-3+((i+1)*CELL_SIZE));
}	

	for (int i = 0; i < 1; i++){
	glNormal3f(-1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0, 0.0, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0, 0.3, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0, 0.3, exitCol-1+((i+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0, 0.0, exitCol-1+((i+1)*CELL_SIZE));
	
	glNormal3f(1.0f, 0.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+4.0, 0.0, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+4.0, 0.3, exitCol-1+(i*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+4.0, 0.3, exitCol-1+((i+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+4.0, 0.0, exitCol-1+((i+1)*CELL_SIZE));
	
	glNormal3f(0.0f, 0.0f, -1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0+((i+1)*CELL_SIZE), 0.0, exitCol-1);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0+((i+1)*CELL_SIZE), 0.3, exitCol-1);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0+(i*CELL_SIZE), 0.3, exitCol-1);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0+(i*CELL_SIZE), 0.0, exitCol-1);
	
	glNormal3f(0.0f, 0.0f, 1.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0+((i+1)*CELL_SIZE), 0.0, exitCol);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0+((i+1)*CELL_SIZE), 0.3, exitCol);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+3.0+(i*CELL_SIZE), 0.3, exitCol);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0+(i*CELL_SIZE), 0.0, exitCol);
	}
	
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0, 0.3, exitCol);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+4.0, 0.3, exitCol);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+4.0, 0.3, exitCol-1.0);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+3.0, 0.3, exitCol-1.0);
	


glEnd();
glDeleteTextures(1, &faltextura);
glBindTexture(GL_TEXTURE_2D,talajtextura);

    for (int x = 0; x < MAZE_WIDTH; x++) {
            for (int z = 0; z < MAZE_HEIGHT; z++) {
		//ground
		glNormal3f(0.0f, 1.0f, 0.0f);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f((x + 1) * CELL_SIZE, 0.0, z * CELL_SIZE);// bal alsó
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x * CELL_SIZE, 0.0, z * CELL_SIZE); //jobb alsó
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x * CELL_SIZE, 0.0, (z + 1) * CELL_SIZE); //jobb felső
		glTexCoord2f(0.0f, 1.0f); glVertex3f((x + 1) * CELL_SIZE, 0.0, (z + 1) * CELL_SIZE); // bal felső
			}
	}
	
		for (int x = 0; x < 6; x++) {
		for (int z = 0; z < 6; z++){
	glNormal3f(0.0f, 1.0f, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH+(x + 1) * CELL_SIZE, 0.0,exitCol-3+(z*CELL_SIZE));
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH+(x * CELL_SIZE), 0.0,exitCol-3+(z*CELL_SIZE));
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH+(x * CELL_SIZE), 0.0, exitCol-3+((z+1)*CELL_SIZE));
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH+(x + 1) * CELL_SIZE, 0.0,exitCol-3+((z+1)*CELL_SIZE));
		}
	}
	
	glDisable(GL_LIGHTING);
glEnd();
glDeleteTextures(1, &talajtextura);

}

void update_camera() {
    vec3 front = {
        .x = cosf(cam.yaw * M_PI / 180.0) * cosf(cam.pitch * M_PI / 180.0),
        .y = sinf(cam.pitch * M_PI / 180.0),
        .z = sinf(cam.yaw * M_PI / 180.0) * cosf(cam.pitch * M_PI / 180.0)
    };
    vec3 right = {
        .x = cosf(cam.yaw * M_PI / 180.0 - M_PI / 2.0),
        .y = 0.0,
        .z = sinf(cam.yaw * M_PI / 180.0 - M_PI / 2.0)
    };
    vec3 up = {
        .x = 0.0,
        .y = 1.0,
        .z = 0.0
    };
	
	
	float cameraRadius = PLAYER_RADIUS;
	    float moveSpeed = MOVE_SPEED;
    if (keys[SDL_SCANCODE_LSHIFT]) {
        moveSpeed *= 1;  
    }
	
    if (keys[SDL_SCANCODE_W]) {
        vec3 next_pos = {
            .x = cam.position.x + front.x * moveSpeed,
            .y = cam.position.y , //+ front.y * moveSpeed,
            .z = cam.position.z + front.z * moveSpeed
        };
        

        // Check for collisions with walls
        int collides_with_wall = 0;
			for (int i = 0; i < wallCount; i++) {
				if (
				next_pos.x + cameraRadius >= wallPositions[i][0][0][0] && next_pos.x - cameraRadius <= wallPositions[i][1][0][0] &&
				next_pos.y + cameraRadius  >= wallPositions[i][0][0][1] && next_pos.y - cameraRadius <= wallPositions[i][1][0][1] &&
				next_pos.z + cameraRadius  >= wallPositions[i][0][0][2] && next_pos.z - cameraRadius <= wallPositions[i][1][0][2]) {
				collides_with_wall = 1;
				break;
				}
			}
        
        // Update position if no collision
		if (!collides_with_wall) {
		// Check if the player walks through the key position
		if (next_pos.x >= secondRow && next_pos.x < secondRow + 1 &&
        next_pos.y >= 0.5 && next_pos.y < 1.5 &&
        next_pos.z >= secondCol && next_pos.z < secondCol + 1) {
        haveKey = 1; // Set haveKey to 1
		}
		cam.position = next_pos;
		performExitActions();
		}
    }
    if (keys[SDL_SCANCODE_S]) {
        vec3 next_pos = {
            .x = cam.position.x - front.x * moveSpeed,
            .y = cam.position.y, // - front.y * moveSpeed,
            .z = cam.position.z - front.z * moveSpeed
        };
		

        // Check for collisions with walls
        int collides_with_wall = 0;
			for (int i = 0; i < wallCount; i++) {
				if (
				next_pos.x + cameraRadius >= wallPositions[i][0][0][0] && next_pos.x - cameraRadius <= wallPositions[i][1][0][0] &&
				next_pos.y + cameraRadius  >= wallPositions[i][0][0][1] && next_pos.y - cameraRadius <= wallPositions[i][1][0][1] &&
				next_pos.z + cameraRadius  >= wallPositions[i][0][0][2] && next_pos.z - cameraRadius <= wallPositions[i][1][0][2]) {
				collides_with_wall = 1;
				break;
				}
			}
        
        // Update position if no collision
		if (!collides_with_wall) {
		// Check if the player walks through the key position
		if (next_pos.x >= secondRow && next_pos.x < secondRow + 1 &&
        next_pos.y >= 0.5 && next_pos.y < 1.5 &&
        next_pos.z >= secondCol && next_pos.z < secondCol + 1) {
        haveKey = 1; // Set haveKey to 1
		}
		cam.position = next_pos;
		performExitActions();
		}
    }
    if (keys[SDL_SCANCODE_A]) {
		
		
        vec3 next_pos = {
            .x = cam.position.x + right.x * moveSpeed,
            .y = cam.position.y,
            .z = cam.position.z + right.z * moveSpeed
        };
		

        // Check for collisions with walls
        int collides_with_wall = 0;
			for (int i = 0; i < wallCount; i++) {
				if (
				next_pos.x + cameraRadius >= wallPositions[i][0][0][0] && next_pos.x - cameraRadius <= wallPositions[i][1][0][0] &&
				next_pos.y + cameraRadius  >= wallPositions[i][0][0][1] && next_pos.y - cameraRadius <= wallPositions[i][1][0][1] &&
				next_pos.z + cameraRadius  >= wallPositions[i][0][0][2] && next_pos.z - cameraRadius <= wallPositions[i][1][0][2]) {
				collides_with_wall = 1;
				break;
				}
			}
        
        // Update position if no collision
		if (!collides_with_wall) {
		// Check if the player walks through the key position
		if (next_pos.x >= secondRow && next_pos.x < secondRow + 1 &&
        next_pos.y >= 0.5 && next_pos.y < 1.5 &&
        next_pos.z >= secondCol && next_pos.z < secondCol + 1) {
        haveKey = 1; // Set haveKey to 1
		}
		cam.position = next_pos;
		performExitActions();
		}
    }
    if (keys[SDL_SCANCODE_D]) {
        vec3 next_pos = {
            .x = cam.position.x - right.x * moveSpeed,
            .y = cam.position.y,
            .z = cam.position.z - right.z * moveSpeed
        };
		

        // Check for collisions with walls
        int collides_with_wall = 0;
			for (int i = 0; i < wallCount; i++) {
				if (
				next_pos.x + cameraRadius >= wallPositions[i][0][0][0] && next_pos.x - cameraRadius <= wallPositions[i][1][0][0] &&
				next_pos.y + cameraRadius  >= wallPositions[i][0][0][1] && next_pos.y - cameraRadius <= wallPositions[i][1][0][1] &&
				next_pos.z + cameraRadius  >= wallPositions[i][0][0][2] && next_pos.z - cameraRadius <= wallPositions[i][1][0][2]) {
				collides_with_wall = 1;
				break;
				}
			}
        
        // Update position if no collision
		if (!collides_with_wall) {
		// Check if the player walks through the key position
		if (next_pos.x >= secondRow && next_pos.x < secondRow + 1 &&
        next_pos.y >= 0.5 && next_pos.y < 1.5 &&
        next_pos.z >= secondCol && next_pos.z < secondCol + 1) {
        haveKey = 1; // Set haveKey to 1
		}
		cam.position = next_pos;
		performExitActions();
		}
    }
		
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cam.position.x, cam.position.y, cam.position.z,
              cam.position.x + front.x, cam.position.y + front.y, cam.position.z + front.z,
              up.x, up.y, up.z);  
}


void drawSkybox() {

GLuint eszak = loadTexture("assets/skybox_north.bmp");
GLuint kelet = loadTexture("assets/skybox_east.bmp");
GLuint del = loadTexture("assets/skybox_south.bmp");
GLuint nyugat = loadTexture("assets/skybox_west.bmp");
GLuint top = loadTexture("assets/skybox_top.bmp");
GLuint bottom = loadTexture("assets/skybox_bottom.bmp");

glDepthMask(GL_FALSE);
glBindTexture(GL_TEXTURE_2D,nyugat);
glBegin(GL_QUADS);
		glNormal3f(-1.0f, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z); //jobb alsó
		glTexCoord2f(1.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z); //jobb felső
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z); //bal felső
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z); // bal alsó
glEnd();		
glDeleteTextures(1, &nyugat);

glBindTexture(GL_TEXTURE_2D, eszak);
glBegin(GL_QUADS);
		glNormal3f(0.0f, 0.0f, -1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &eszak);

glBindTexture(GL_TEXTURE_2D, kelet);
glBegin(GL_QUADS);
		glNormal3f(1.0f, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &kelet);

glBindTexture(GL_TEXTURE_2D, del);
glBegin(GL_QUADS);
		glNormal3f(0.0f, 0.0f, 1.0f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &del);

glBindTexture(GL_TEXTURE_2D, top);
glBegin(GL_QUADS);
		glNormal3f(0.0f, -1.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &top);

glBindTexture(GL_TEXTURE_2D, bottom);
glBegin(GL_QUADS);
		glNormal3f(0.0f, 1.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &bottom);
glDepthMask(GL_TRUE);	

}
void initializeMaze() {
    for (int i = 0; i < MAZE_HEIGHT; i++) {
        for (int j = 0; j < MAZE_WIDTH; j++) {
            maze[i][j] = WALL;
        }
    }
}

bool isValidCell(int x, int y) {
    return x >= 0 && x < MAZE_WIDTH && y >= 0 && y < MAZE_HEIGHT && maze[y][x] == WALL;
}

bool hasUnvisitedNeighbors(int x, int y) {
    return (isValidCell(x - 2, y) || isValidCell(x + 2, y) || isValidCell(x, y - 2) || isValidCell(x, y + 2));
}

void carvePath(int x, int y) {
    maze[y][x] = ROUTE;

    int dx, dy;
    int direction[4][2] = { {2, 0}, {-2, 0}, {0, 2}, {0, -2} };
    int shuffle[4] = { 0, 1, 2, 3 };

    // Randomize the order of directions
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = shuffle[i];
        shuffle[i] = shuffle[j];
        shuffle[j] = temp;
    }

    for (int i = 0; i < 4; i++) {
        int dir = shuffle[i];
        dx = x + direction[dir][0];
        dy = y + direction[dir][1];

        if (isValidCell(dx, dy) && hasUnvisitedNeighbors(dx, dy)) {
            maze[dy][dx] = ROUTE;
            maze[(y + dy) / 2][(x + dx) / 2] = ROUTE;
            carvePath(dx, dy);
        }
    }
}

void generateMaze() {
    initializeMaze();

    srand(time(NULL));
    int startX = rand() % (MAZE_WIDTH / 2) * 2 + 1;
    int startY = rand() % (MAZE_HEIGHT / 2) * 2 + 1;

    carvePath(startX, startY);
	
    for (int x = 0; x < MAZE_WIDTH; x++) {
            for (int z = 0; z < MAZE_HEIGHT; z++) {
				maze[x][z]==maze[x][z];
                if (maze[x][z] == WALL && wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
						wallPositions[wallCount][0][0][0] = x;
						wallPositions[wallCount][0][0][1] = 0;
						wallPositions[wallCount][0][0][2] = z;
						wallPositions[wallCount][1][0][0] = x + 1;
						wallPositions[wallCount][1][0][1] = 1;
						wallPositions[wallCount][1][0][2] = z + 1;
						wallCount++;
				}		
			}
	}

 	for (int i = 0; i <= MAZE_WIDTH-1; i++) {
						if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH){
						wallPositions[wallCount][0][0][0] = i;
						wallPositions[wallCount][0][0][1] = 0.0f;
						wallPositions[wallCount][0][0][2] = MAZE_HEIGHT;
						wallPositions[wallCount][1][0][0] = i + 1;
						wallPositions[wallCount][1][0][1] = 1;
						wallPositions[wallCount][1][0][2] = MAZE_HEIGHT;
						wallCount++;
					}
	}

for (int i = 0; i < MAZE_WIDTH; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = i;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = 0.0f;
        wallPositions[wallCount][1][0][0] = i + 1;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = 0.0f;
        wallCount++;
    }
}

for (int i = 0; i < MAZE_HEIGHT; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = 0.0f;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = i;
        wallPositions[wallCount][1][0][0] = 0.0f;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = i + 1;
        wallCount++;
    }
}

for (int i = 0; i < exitCol - 1; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = i ;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = i + 1;
        wallCount++;
    }
}

for (int i = exitCol; i < MAZE_HEIGHT; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = i;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = i + 1;
        wallCount++;
    }
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol - 0.75;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 1.0;
    wallCount++;
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 0.25;
    wallCount++;
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol - 0.75;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 0.75;
    wallCount++;
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol - 0.25;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH+1.0;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 0.25;
    wallCount++;
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol - 0.75;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 1.0;
    wallCount++;
}

if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
    wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][0][0][1] = 0.0f;
    wallPositions[wallCount][0][0][2] = exitCol;
    wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0;
    wallPositions[wallCount][1][0][1] = 1.0f;
    wallPositions[wallCount][1][0][2] = exitCol - 0.25;
    wallCount++;
}

for (int i = 1; i < 3; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 1 + i;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = exitCol - 1 + i + 1;
        wallCount++;
    }

    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 4 + i;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = exitCol - 4 + i + 1;
        wallCount++;
    }
}

for (int i = 0; i < 5; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0 + i + 1;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 3;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0 + i ;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = exitCol - 3;
        wallCount++;
    }

    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 1.0 + i + 1;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol + 2;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 1.0 + i ;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = exitCol + 2;
        wallCount++;
    }

    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 6.0;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 3 + i;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 6.0;
        wallPositions[wallCount][1][0][1] = 1.0f;
        wallPositions[wallCount][1][0][2] = exitCol - 3 + i + 1;
        wallCount++;
    }
}

for (int i = 0; i < 1; i++) {
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 3.0;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 1 + i;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 3.0;
        wallPositions[wallCount][1][0][1] = 0.3f;
        wallPositions[wallCount][1][0][2] = exitCol - 1 + i +1;
        wallCount++;
    }

    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 4.0;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 1 + i ;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 4.0;
        wallPositions[wallCount][1][0][1] = 0.3f;
        wallPositions[wallCount][1][0][2] = exitCol - 1 + i + 1;
        wallCount++;
    }
	
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 3.0 + i + 1 ;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol - 1;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 3.0 + i ;
        wallPositions[wallCount][1][0][1] = 0.3f;
        wallPositions[wallCount][1][0][2] = exitCol - 1;
        wallCount++;
    }
	
    if (wallCount < MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH) {
        wallPositions[wallCount][0][0][0] = MAZE_WIDTH + 3.0 + i + 1;
        wallPositions[wallCount][0][0][1] = 0.0f;
        wallPositions[wallCount][0][0][2] = exitCol;
        wallPositions[wallCount][1][0][0] = MAZE_WIDTH + 3.0 + i;
        wallPositions[wallCount][1][0][1] = 0.3f;
        wallPositions[wallCount][1][0][2] = exitCol;
        wallCount++;
    }
}
	
}

void randomPickRoute(int maze[][MAZE_WIDTH], int *row, int *col) {
    srand(time(NULL)); // Initialize the random number generator

    int found = 0; // Flag variable to control the search process

    while (!found) {
        int randomRow = rand() % MAZE_HEIGHT;
        int randomCol = rand() % MAZE_WIDTH;

        if (maze[randomRow][randomCol] == ROUTE) {
            found = 1; // Set the flag to indicate the route is found
            *row = randomRow;
            *col = randomCol;
				cam.position.x = randomRow*CELL_SIZE+0.5;
				cam.position.y = 0.5;
				cam.position.z = randomCol*CELL_SIZE+0.5;
        }
    }
}

void randomPickFarRoute(int maze[][MAZE_WIDTH], int firstRow, int firstCol, int *row, int *col) {
    srand(time(NULL)); // Initialize the random number generator

    int found = 0; // Flag variable to control the search process

    while (!found) {
        int randomRow = rand() % MAZE_HEIGHT;
        int randomCol = rand() % MAZE_WIDTH;

        if (maze[randomRow][randomCol] == ROUTE &&
            abs(randomRow - firstRow) > 3 && abs(randomCol - firstCol) > 3) {
            found = 1; // Set the flag to indicate the far route is found
            *row = randomRow;
            *col = randomCol;
        }
    }
}

void randomPickExitRoute(int maze[][MAZE_WIDTH], int *row, int *col) {
    srand(time(NULL)); // Initialize the random number generator

    int found = 0; // Flag variable to control the search process

    while (!found) {
        int randomCol = rand() % MAZE_HEIGHT;

        if (maze[20][randomCol] == ROUTE) {
            found = 1; // Set the flag to indicate the route is found
            *row = MAZE_WIDTH;
            *col = randomCol;
            return;
        }
    }
}

void draw_help(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
	glDisable(GL_LIGHTING);
	
    GLuint help = loadTexture("assets/help.bmp");
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, help);
    	
    glBegin(GL_QUADS);
	glColor3f(1.0, 1.0, 1.0);
    glTexCoord2f(0, 1);
    glVertex2f(0, 0);
    
    glTexCoord2f(1, 1);
    glVertex2f(WIDTH, 0);
    
    glTexCoord2f(1,0);
    glVertex2f(WIDTH, HEIGHT);
    
    glTexCoord2f(0, 0);
    glVertex2f(0, HEIGHT);
    glEnd();
    
    glDeleteTextures(1, &help);
    
	glEnable(GL_LIGHTING);
	
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void draw_key(Object* key, float time)
{
	key->position.x = secondRow;
	key->position.z = secondCol;
	float scale = 0.005f;
	
	float translation_distance = 0.1f;
	float translation_offset = translation_distance * sinf(time);
	
	glPushMatrix();
		glTranslatef(key->position.x+0.5, 0.4+translation_offset, key->position.z+0.5);
		glBindTexture(GL_TEXTURE_2D, key->texture);
		glScalef(scale, scale, scale);
		draw_model(&key->model);
	glPopMatrix();
}


void performExitActions() {
    // Check if the player has the key
    if (haveKey == 1) {
        // Check if the player is close to the exit
        if (isPlayerCloseToExit() && !exitActionsPerformed) {
            // Perform actions when the player is close to the exit with the key
            printf("Congratulations! You have escaped the maze!\n");
			exitActionsPerformed = 1;
        }
    }
}

float calculate_door_width(const Model* model) {
    float minX = FLT_MAX;
    float maxX = -FLT_MAX;

    for (int i = 0; i < model->n_vertices; ++i) {
        float x = model->vertices[i].x;
        if (x < minX)
            minX = x;
        if (x > maxX)
            maxX = x;
    }

    return maxX - minX;
}

void draw_door(Object* door, float time) {
    door->position.x = exitRow;
    door->position.z = exitCol;

    float scale = 0.005f;
    float doorWidth = calculate_door_width(&door->model);

    glPushMatrix();
    glTranslatef(door->position.x+0.075, 0, door->position.z - 0.5);
    glBindTexture(GL_TEXTURE_2D, door->texture);
    glRotatef(-90, 0, 1, 0);
    glScalef(scale, scale, scale);

    if (doorOpen) {
        float pivotX = doorWidth / 2.0f;
        float targetRotation = -90.0f;

        if (doorRotation > targetRotation) {
            float angle = doorRotation - time * 0.5f;
            if (angle < targetRotation)
                angle = targetRotation;

            glTranslatef(doorWidth - pivotX, 0.0f, 0.0f);  // Translate to the hinges position
            glRotatef(angle, 0, 1, 0);  // Rotate around the Y-axis
            glTranslatef(-doorWidth + pivotX, 0.0f, 0.0f);  // Translate back to the original position

            doorRotation = angle;
        }
        else {
            // Keep the door open at -90 degrees
            glTranslatef(doorWidth - pivotX, 0.0f, 0.0f);  // Translate to the hinges position
            glRotatef(targetRotation, 0, 1, 0);  // Rotate around the Y-axis
            glTranslatef(-doorWidth + pivotX, 0.0f, 0.0f);  // Translate back to the original position
        }
    }

    draw_model(&door->model);
    glPopMatrix();
}

void draw_cup(Object* cup, float time)
{
	cup->position.x = MAZE_WIDTH+3.5;
	cup->position.y = 4;
	cup->position.z = exitCol-0.5;
	float scale = 0.4f;

	glPushMatrix();
		glTranslatef(cup->position.x, 1.45, cup->position.z);
		glBindTexture(GL_TEXTURE_2D, cup->texture);
		glRotatef(90, 0, 1, 0);
		glRotatef(-90, 1, 0, 0);
		glScalef(scale, scale, scale);
		glColor3f(0.855, 0.647, 0.125);
		draw_model(&cup->model);
	glPopMatrix();
}

int main(int argc, char **argv) {
SDL_Init(SDL_INIT_EVERYTHING);
SDL_Window *window = SDL_CreateWindow("Belus László Grafika beadandó feladat", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
SDL_GLContext context = SDL_GL_CreateContext(window);
SDL_SetRelativeMouseMode(true);
glMatrixMode(GL_PROJECTION);
glLoadIdentity();
gluPerspective(FOV, (float) WIDTH / HEIGHT, NEAR_CLIP, FAR_CLIP);
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();
glEnable(GL_DEPTH_TEST); 
glEnable(GL_LIGHTING);
glEnable(GL_LIGHT0);
    
clock_t start_time = clock();
srand(time(NULL));
    generateMaze();
    randomPickRoute(maze, &firstRow, &firstCol);
	randomPickFarRoute(maze, firstRow, firstCol, &secondRow, &secondCol);
	randomPickExitRoute(maze, &exitRow, &exitCol);
	
	Object key;
	load_model(&key.model, "data/models/key2.obj");
	key.texture = loadTexture("data/textures/key_specular.bmp");

	Object door;
	load_model(&door.model, "data/models/door.obj");
	door.texture = loadTexture("data/textures/door.bmp");

	Object cup;
	load_model(&cup.model, "data/models/trophy.obj");
	
	init();
while (1) {
	clock_t current_time = clock();
    float elapsed_time = (float)(current_time - start_time) / CLOCKS_PER_SEC;
	
handle_events();
update_camera();
glClearColor(0.5, 0.5, 1.0, 1.0);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   drawMaze();
	draw_cup(&cup, elapsed_time);
   if (!haveKey){
    draw_key(&key, elapsed_time);
   }
   
   if (!exitActionsPerformed){
	   doorOpen = false;
   }
   else doorOpen = true;
   
		if(isHelpOn)
	{
		draw_help();
	}
	draw_door(&door, elapsed_time);
drawSkybox();
SDL_GL_SwapWindow(window);
}
SDL_GL_DeleteContext(context);
SDL_DestroyWindow(window);
SDL_Quit();
return 0;
}
