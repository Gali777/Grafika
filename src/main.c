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

#define WIDTH 1024
#define HEIGHT 768
#define FOV 90.0
#define NEAR_CLIP 0.1
#define FAR_CLIP 100.0
#define MOVE_SPEED 0.25
#define MOUSE_SENSITIVITY 0.2

#define MAZE_WIDTH 40 //X
#define MAZE_HEIGHT 40 //Z
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

typedef struct {
    int x;
    int y;
    int z;
} WallCoordinate;

WallCoordinate wallCoordinates[MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH];
int numWalls = 0;

int wallPositions[MAZE_WIDTH * MAZE_HEIGHT * MAZE_DEPTH][2][2][3];
int wallCount = 0;

int maze[MAZE_HEIGHT][MAZE_WIDTH];

	GLfloat light_position[] = {20.0, 5.0, 20.0}; //1.0};
	GLfloat light_ambient[] = { 1.0, 1.0, 1.0, 1.0};
	GLfloat light_diffuse[] = {1.0, 1.0, 1.0, 1.0};
	GLfloat light_specular[] = {1.0, 1.0, 1.0, 32.0};

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
int haveKey = 0;

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

void drawMaze() {
	GLfloat sun_direction[] = { sunlight.direction[0], sunlight.direction[1], sunlight.direction[2], 0.0 };
	GLfloat sun_intensity[] = { sunlight.intensity, sunlight.intensity, sunlight.intensity, 1.0 };

	glLightfv(GL_LIGHT0, GL_POSITION, sun_direction);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, sun_intensity);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	
	GLfloat material_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };  // Object color

	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material_diffuse);
	
GLuint faltextura = loadTexture("assets/brickwall.bmp");
GLuint talajtextura = loadTexture("assets/ground.bmp");

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
					
                    glBegin(GL_QUADS);
					//glColor3f(1, 1, 1);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmin, 0, zmin);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmax, 0, zmin);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmax, MAZE_DEPTH, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmin);
                    glEnd();
					
                    // Draw back face
					
                    glBegin(GL_QUADS);
					//glColor3f(0.25, 0.25, 0.25);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmin, 0, zmax);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmax, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmax, 0, zmax);
                    glEnd();
					
                    // Draw left face
					
                    glBegin(GL_QUADS);
					//glColor3f(0.5, 0.5, 0.5);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmin, 0, zmin);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmin, MAZE_DEPTH, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmin, 0, zmax);
                    glEnd();
					
					
                    // Draw right face
					
                    glBegin(GL_QUADS);
					//glColor3f(0.5, 0.5, 0.5);
                    glTexCoord2f(1.0f, 1.0f); glVertex3f(xmax, 0, zmin);
                    glTexCoord2f(0.0f, 1.0f); glVertex3f(xmax, 0, zmax);
                    glTexCoord2f(0.0f, 0.0f); glVertex3f(xmax, MAZE_DEPTH, zmax);
                    glTexCoord2f(1.0f, 0.0f); glVertex3f(xmax, MAZE_DEPTH, zmin);
                    glEnd();
					
					// Draw top face
					glBegin(GL_QUADS);
					//GlColor3f(0.5, 0.5, 0.5);
					glVertex3f(xmin, MAZE_DEPTH, zmin);
					glVertex3f(xmax, MAZE_DEPTH, zmin);
					glVertex3f(xmax, MAZE_DEPTH, zmax);
					glVertex3f(xmin, MAZE_DEPTH, zmax);
					
					// Draw bottom face
					glBegin(GL_QUADS);
					//glColor3f(0.5, 0.5, 0.5);
					glVertex3f(xmin,0, zmin);
					glVertex3f(xmax,0, zmin);
					glVertex3f(xmax,0, zmax);
					glVertex3f(xmin,0, zmax);
				}
			}
		}
	
	
for (int i = 0; i <= MAZE_WIDTH-1; i++) {
    //észak
    //glColor3f(1, 1, 1);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(i * CELL_SIZE, 0.0f, MAZE_HEIGHT);
    glTexCoord2f(1.0f, 0.0f); glVertex3f((i+1) * CELL_SIZE, 0.0f, MAZE_HEIGHT);
    glTexCoord2f(1.0f, 1.0f); glVertex3f((i+1) * CELL_SIZE, MAZE_DEPTH, MAZE_HEIGHT);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(i * CELL_SIZE, MAZE_DEPTH, MAZE_HEIGHT);
}

for (int i = 0; i < MAZE_WIDTH; i++) {
    //dél
    //glColor3f(0.25, 0.25, 0.25);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(i * CELL_SIZE, 0.0, 0.0);
    glTexCoord2f(1.0f, 0.0f); glVertex3f((i + 1) * CELL_SIZE, 0.0, 0.0);
    glTexCoord2f(1.0f, 1.0f); glVertex3f((i + 1) * CELL_SIZE, MAZE_DEPTH, 0.0);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(i * CELL_SIZE, MAZE_DEPTH, 0.0);
}

for (int i = 0; i < MAZE_HEIGHT; i++) {
    //kelet
    //glColor3f(0.5, 0.5, 0.5);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(0.0, 0.0, i*CELL_SIZE);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(0.0, MAZE_DEPTH, i*CELL_SIZE);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0, MAZE_DEPTH, (i+1)*CELL_SIZE);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0, 0.0, (i+1)*CELL_SIZE);
}

for (int i = 0; i < MAZE_HEIGHT; i++) {
    //nyugat
    glColor3f(0.5, 0.5, 0.5);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, i*CELL_SIZE);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, i*CELL_SIZE);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(MAZE_WIDTH, MAZE_DEPTH, (i+1)*CELL_SIZE);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(MAZE_WIDTH, 0.0, (i+1)*CELL_SIZE);
}
glEnd();
glDeleteTextures(1, &faltextura);
glBindTexture(GL_TEXTURE_2D,talajtextura);

    for (int x = 0; x < MAZE_WIDTH; x++) {
            for (int z = 0; z < MAZE_HEIGHT; z++) {
		//talaj
		glBegin(GL_QUADS);
		//glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(0.0f, 0.0f); glVertex3f((x + 1) * CELL_SIZE, 0.0, z * CELL_SIZE);// bal alsó
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x * CELL_SIZE, 0.0, z * CELL_SIZE); //jobb alsó
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x * CELL_SIZE, 0.0, (z + 1) * CELL_SIZE); //jobb felső
		glTexCoord2f(0.0f, 1.0f); glVertex3f((x + 1) * CELL_SIZE, 0.0, (z + 1) * CELL_SIZE); // bal felső
			}
	}
	
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
        moveSpeed *= 0.2;  
    }
	
    if (keys[SDL_SCANCODE_W]) {
        vec3 next_pos = {
            .x = cam.position.x + front.x * moveSpeed,
            .y = cam.position.y + front.y * moveSpeed,
            .z = cam.position.z + front.z * moveSpeed
        };
        
/*        // Check if next position is within the maze boundaries
        if (next_pos.x - cameraRadius < 0 || next_pos.x + cameraRadius >= MAZE_WIDTH ||
            next_pos.y - cameraRadius < 0 || next_pos.y + cameraRadius >= MAZE_DEPTH ||
            next_pos.z - cameraRadius < 0 || next_pos.z + cameraRadius >= MAZE_HEIGHT) {
            return;
        } */
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
		}
    }
    if (keys[SDL_SCANCODE_S]) {
        vec3 next_pos = {
            .x = cam.position.x - front.x * moveSpeed,
            .y = cam.position.y - front.y * moveSpeed,
            .z = cam.position.z - front.z * moveSpeed
        };
		
/*        // Check if next position is within the maze boundaries
        if (next_pos.x - cameraRadius < 0 || next_pos.x + cameraRadius >= MAZE_WIDTH ||
            next_pos.y - cameraRadius < 0 || next_pos.y + cameraRadius >= MAZE_DEPTH ||
            next_pos.z - cameraRadius < 0 || next_pos.z + cameraRadius >= MAZE_HEIGHT) {
            return;
        } */
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
		}
    }
    if (keys[SDL_SCANCODE_A]) {
		
		
        vec3 next_pos = {
            .x = cam.position.x + right.x * moveSpeed,
            .y = cam.position.y,
            .z = cam.position.z + right.z * moveSpeed
        };
		
/*        // Check if next position is within the maze boundaries
        if (next_pos.x - cameraRadius < 0 || next_pos.x + cameraRadius >= MAZE_WIDTH ||
            next_pos.y - cameraRadius < 0 || next_pos.y + cameraRadius >= MAZE_DEPTH ||
            next_pos.z - cameraRadius < 0 || next_pos.z + cameraRadius >= MAZE_HEIGHT) {
            return;
        } */
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
		}
    }
    if (keys[SDL_SCANCODE_D]) {
        vec3 next_pos = {
            .x = cam.position.x - right.x * moveSpeed,
            .y = cam.position.y,
            .z = cam.position.z - right.z * moveSpeed
        };
		
/*        // Check if next position is within the maze boundaries
        if (next_pos.x - cameraRadius < 0 || next_pos.x + cameraRadius >= MAZE_WIDTH ||
            next_pos.y - cameraRadius < 0 || next_pos.y + cameraRadius >= MAZE_DEPTH ||
            next_pos.z - cameraRadius < 0 || next_pos.z + cameraRadius >= MAZE_HEIGHT) {
            return;
        } */
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
		}
    }
		
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cam.position.x, cam.position.y, cam.position.z,
              cam.position.x + front.x, cam.position.y + front.y, cam.position.z + front.z,
              up.x, up.y, up.z);  
}


void displayCameraPosition() {
    double modelViewMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelViewMatrix);

    double cameraX = modelViewMatrix[12];
    double cameraY = modelViewMatrix[13];
    double cameraZ = modelViewMatrix[14];

	float pitch = cam.pitch;
    float yaw = cam.yaw;
	
    //printf("Camera Position: X=%.2f, Y=%.2f, Z=%.2f\n", cameraX, cameraY, cameraZ);
	//printf("Camera Pitch: %.2f\n", pitch);
    //printf("Camera Yaw: %.2f\n", yaw);
	
GLuint eszak = loadTexture("assets/skybox_north.bmp");
GLuint kelet = loadTexture("assets/skybox_east.bmp");
GLuint del = loadTexture("assets/skybox_south.bmp");
GLuint nyugat = loadTexture("assets/skybox_west.bmp");
GLuint top = loadTexture("assets/skybox_top.bmp");
GLuint bottom = loadTexture("assets/skybox_bottom.bmp");

glDepthMask(GL_FALSE);
glBindTexture(GL_TEXTURE_2D,nyugat);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z); //jobb alsó
		glTexCoord2f(1.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z); //jobb felső
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z); //bal felső
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z); // bal alsó
glEnd();		
glDeleteTextures(1, &nyugat);

glBindTexture(GL_TEXTURE_2D, eszak);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &eszak);

glBindTexture(GL_TEXTURE_2D, kelet);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, 55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &kelet);

glBindTexture(GL_TEXTURE_2D, del);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, -55.0 + cam.position.y, -55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &del);

glBindTexture(GL_TEXTURE_2D, top);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, 55.0 + cam.position.z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(-55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(55.0 + cam.position.x, 55.0 + cam.position.y, -55.0 + cam.position.z);
glEnd();		
glDeleteTextures(1, &top);

glBindTexture(GL_TEXTURE_2D, bottom);
glBegin(GL_QUADS);
		glColor3f(1.0, 1.0, 1.0);
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

void draw_door(Object* door, float time)
{
	float scale = 0.005f;
	
	glPushMatrix();
		//glTranslatef(door->position.x, 0.4, door->position.z);
		glBindTexture(GL_TEXTURE_2D, door->texture);
		glScalef(scale, scale, scale);
		draw_model(&door->model);
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
    printf("First Route: (%d, %d)\n", firstRow, firstCol);

	randomPickFarRoute(maze, firstRow, firstCol, &secondRow, &secondCol);
	printf("Second Far Route: (%d, %d)\n", secondRow, secondCol);
	
	sunlight.direction[0] = 5.0f;  // X direction
	sunlight.direction[1] = 5.0f; // Y direction
	sunlight.direction[2] = 5.0f; // Z direction
	sunlight.intensity = 0.5f;     // Intensity (adjust as needed)
	
glEnable(GL_TEXTURE_2D);

	Object key;
	
	load_model(&key.model, "data/models/key2.obj");
	key.texture = loadTexture("data/textures/key_specular.bmp");
		
	Object door;
	
	load_model(&door.model, "data/models/door.obj");
	door.texture = loadTexture("data/textures/door.bmp");
	door.position.x = 1;
    door.position.y = 2;
    door.position.z = 1;	
	
while (1) {
	clock_t current_time = clock();
    float elapsed_time = (float)(current_time - start_time) / CLOCKS_PER_SEC;
handle_events();

update_camera();
glClearColor(0.5, 0.5, 1.0, 1.0);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   drawMaze();
   if (!haveKey){
    draw_key(&key, elapsed_time);
   }
		if(isHelpOn)
	{
		draw_help();
	}
	draw_door(&door, elapsed_time);
displayCameraPosition();
SDL_GL_SwapWindow(window);
}


SDL_GL_DeleteContext(context);
SDL_DestroyWindow(window);
SDL_Quit();

return 0;
}