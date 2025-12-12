#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>

// Window dimensions (NON-CONST: Allow resize updates)
int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;

// Animation and camera variables
float animationTime = 0.0f;
float zoom = 1.0f;
float cameraAngleX = 0.0f; // Mouse rotation on X
float cameraAngleY = 0.0f; // Mouse rotation on Y
float cameraX = 0.0f;      // Camera translation X
float cameraY = 0.0f;      // Camera translation Y
float cameraZ = -20.0f;    // Camera translation Z (zoom via translation)
bool mousePressed = false;
int lastMouseX = 0, lastMouseY = 0; // Renamed to avoid conflict
bool paused = false; // For pause functionality

// HUD Button structure
struct Button {
    float x, y;             // Position (bottom-left)
    float width, height;    // Size
    std::string label;      // Text
    bool pressed;           // State
    void (*action)();       // Function to call on press
};

// Simple function pointers for buttons
void zoomIn() { zoom *= 1.1f; glutPostRedisplay(); }
void zoomOut() { zoom /= 1.1f; if (zoom < 0.1f) zoom = 0.1f; glutPostRedisplay(); }
void resetView() {
    zoom = 1.0f;
    cameraAngleX = cameraAngleY = 0.0f;
    cameraX = cameraY = 0.0f;
    cameraZ = -20.0f;
    glutPostRedisplay();
}
void togglePause() { paused = !paused; glutPostRedisplay(); }

// Vector of buttons (positioned in bottom-right HUD) - FIXED: Explicit float literals/casts to avoid narrowing warnings
std::vector<Button> buttons = {
    {static_cast<float>(WINDOW_WIDTH - 120), 10.0f, 100.0f, 30.0f, "Zoom In", false, zoomIn},
    {static_cast<float>(WINDOW_WIDTH - 120), 50.0f, 100.0f, 30.0f, "Zoom Out", false, zoomOut},
    {static_cast<float>(WINDOW_WIDTH - 120), 90.0f, 100.0f, 30.0f, "Reset", false, resetView},
    {static_cast<float>(WINDOW_WIDTH - 120), 130.0f, 100.0f, 30.0f, "Pause", false, togglePause}
};

// Moon structure
struct Moon {
    float distance;     // Distance from planet
    float size;         // Radius
    float orbitSpeed;   // Orbital speed around planet
    float red, green, blue; // Color (3 components)
};

// Planet structure (enhanced with moons and rings)
struct Planet {
    float distance;     // Distance from sun
    float size;         // Radius
    float orbitSpeed;   // Orbital speed (degrees per frame)
    float rotationSpeed; // Self-rotation speed (degrees per frame)
    float red, green, blue; // Color
    std::string name;   // Name for labeling
    std::vector<Moon> moons; // Moons
    bool hasRings;      // For Saturn
};

// Stars for background (expanded for more "noise")
std::vector<std::pair<float, float>> stars = {
    // Existing stars
    {-15.0f, -10.0f}, {18.0f, 5.0f}, {-12.0f, 8.0f}, {10.0f, -12.0f},
    {5.0f, 15.0f}, {-8.0f, -5.0f}, {20.0f, 2.0f}, {-3.0f, 18.0f},
    {16.0f, -8.0f}, {-10.0f, 12.0f}, {7.0f, -15.0f}, {-16.0f, 6.0f},
    // Additional stars for more noise/density
    { -2.0f, -2.0f}, {4.0f, -6.0f}, {12.0f, 10.0f}, {-18.0f, 3.0f},
    {8.0f, -18.0f}, {-5.0f, 7.0f}, {14.0f, -4.0f}, { -9.0f, 14.0f},
    {1.0f, -9.0f}, { -14.0f, -1.0f}, {6.0f, 11.0f}, {19.0f, -7.0f},
    { -7.0f,  -3.0f}, {3.0f, 4.0f}, {11.0f, -14.0f}, { -1.0f, 16.0f},
    {17.0f, 1.0f}, { -6.0f, -8.0f}, {9.0f, 6.0f}, { -11.0f, 9.0f},
    {2.0f, -16.0f}, {15.0f, -2.0f}, { -4.0f, 12.0f}, {13.0f,  -5.0f}
};

// Planets vector (unchanged from previous)
std::vector<Planet> planets = {
    // Mercury
    {2.0f, 0.2f, 4.0f, 2.0f, 0.8f, 0.6f, 0.4f, "Mercury", {}, false},
    // Venus
    {3.0f, 0.3f, 2.5f, 1.5f, 1.0f, 0.8f, 0.4f, "Venus", {}, false},
    // Earth (with Moon)
    {4.5f, 0.3f, 1.8f, 1.0f, 0.2f, 0.5f, 1.0f, "Earth",
        {{0.8f, 0.05f, 0.5f, 0.5f, 0.8f, 1.0f}}, false
    },
    // Mars
    {5.5f, 0.25f, 1.2f, 0.8f, 1.0f, 0.4f, 0.2f, "Mars", {}, false},
    // Jupiter (with 2 moons)
    {8.0f, 0.8f, 0.6f, 0.5f, 1.0f, 0.7f, 0.3f, "Jupiter",
        {
            {1.2f, 0.1f, 1.0f, 0.8f, 0.8f, 0.7f},
            {1.5f, 0.08f, 0.9f, 0.7f, 0.6f, 0.5f}
        },
        false
    },
    // Saturn (with rings and 1 moon)
    {10.0f, 0.7f, 0.4f, 0.4f, 0.9f, 0.8f, 0.5f, "Saturn",
        {{1.0f, 0.06f, 0.8f, 0.8f, 0.6f, 0.4f}},
        true
    },
    // Uranus
    {12.0f, 0.5f, 0.3f, 0.3f, 0.4f, 0.8f, 1.0f, "Uranus", {}, false},
    // Neptune
    {14.0f, 0.5f, 0.2f, 0.2f, 0.2f, 0.4f, 1.0f, "Neptune", {}, false}
};

// Drawing functions
void drawSphere(float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glutSolidSphere(radius, 20, 20);
}

// Enhanced Sun with procedural gradient (yellow core to orange/red edges, mimicking solar gradient)
void drawSun() {
    glPushMatrix();
    // Core: Bright yellow
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.6f, 20, 20);

    // Mid layer: Yellow-orange gradient
    for (int layer = 1; layer <= 4; ++layer) {
        float rad = 0.6f + (layer * 0.1f);
        float orangeIntensity = layer * 0.2f;
        glColor3f(1.0f, 1.0f - orangeIntensity, 0.0f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f); // Center
        glutSolidSphere(rad, 20, 20);
        glPopMatrix();
    }

    // Outer flare: Subtle red/orange rings for texture
    glDisable(GL_LIGHTING); // For additive effect
    for (int i = 0; i < 8; ++i) {
        float angle = (animationTime * 2.0f) + (i * 45.0f);
        float rad = 1.0f + (sin(angle * 0.01f) * 0.1f); // Pulsing
        glColor4f(1.0f, 0.5f, 0.0f, 0.3f); // Semi-transparent orange
        glPushMatrix();
        glRotatef(angle, 0.0f, 1.0f, 0.0f);
        glTranslatef(rad, 0.0f, 0.0f);
        glutSolidSphere(0.05f, 8, 8); // Small flare spots
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawOrbit(float radius) {
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float angle = 2.0f * M_PI * i / 100.0f;
        glVertex3f(radius * cos(angle), 0.0f, radius * sin(angle));
    }
    glEnd();
}

void drawRings(float innerRadius, float outerRadius) {
    glColor3f(0.8f, 0.8f, 0.7f);
    int segments = 50;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = innerRadius * cos(angle);
        float z = innerRadius * sin(angle);
        glVertex3f(x, 0.01f, z);
        x = outerRadius * cos(angle);
        z = outerRadius * sin(angle);
        glVertex3f(x, 0.01f, z);
    }
    glEnd();
}

void drawMoon(const Moon& moon, float planetAngle) {
    glPushMatrix();
    glRotatef(animationTime * moon.orbitSpeed + planetAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(moon.distance, 0.0f, 0.0f);
    drawSphere(moon.size, moon.red, moon.green, moon.blue);
    glPopMatrix();
}

// Enhanced stars with more noise (random twinkling + density)
void drawStars() {
    glDisable(GL_LIGHTING);
    glBegin(GL_POINTS);
    glPointSize(1.5f + (sin(animationTime * 0.05f) * 0.5f)); // Global twinkle
    for (const auto& star : stars) {
        // Individual twinkle variation
        float twinkle = sin(animationTime * 0.1f + star.first * 0.01f) * 0.5f + 0.5f;
        glColor3f(1.0f, 1.0f, 1.0f * twinkle);
        glVertex3f(star.first * zoom, 5.0f + sin(animationTime * 0.1f + star.second * 0.01f), star.second * zoom);
    }
    // Add extra noise particles (small, faint stars)
    for (int i = 0; i < 50; ++i) {
        float rx = (rand() % 4000 - 2000) / 100.0f; // Random X -20 to 20
        float rz = (rand() % 4000 - 2000) / 100.0f; // Random Z
        float twinkle = (rand() % 100) / 100.0f; // 0-1
        glColor3f(0.8f, 0.8f, 1.0f * twinkle); // Bluish faint
        glVertex3f(rx * zoom, 10.0f + (rand() % 1000 - 500) / 100.0f, rz * zoom);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawLabel(const std::string& name, float x, float y, float z) {
    glRasterPos3f(x, y, z);
    for (char c : name) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}

// Draw HUD Buttons (2D overlay)
void drawHUD() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    for (auto& btn : buttons) {
        // Highlight if pressed
        if (btn.pressed) {
            glColor3f(0.4f, 0.4f, 0.4f); // Lighter gray when pressed
        }
        else {
            glColor3f(0.2f, 0.2f, 0.2f); // Dark gray
        }
        glBegin(GL_QUADS);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.width, btn.y);
        glVertex2f(btn.x + btn.width, btn.y + btn.height);
        glVertex2f(btn.x, btn.y + btn.height);
        glEnd();

        // Draw border
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.width, btn.y);
        glVertex2f(btn.x + btn.width, btn.y + btn.height);
        glVertex2f(btn.x, btn.y + btn.height);
        glEnd();

        // Draw label
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(btn.x + 10, btn.y + 10);
        for (char c : btn.label) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
        }
    }

    // Status text (e.g., paused?)
    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2f(10, WINDOW_HEIGHT - 30);
    std::string status = paused ? "PAUSED" : "Running";
    for (char c : status) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Display callback (enhanced with HUD and camera translation)
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply camera transformations
    glTranslatef(cameraX, cameraY, cameraZ);
    glScalef(zoom, zoom, zoom);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);

    // Draw background stars (with noise)
    drawStars();

    // Draw orbital paths
    for (const auto& p : planets) {
        drawOrbit(p.distance);
    }

    // Draw sun (enhanced gradient)
    drawSun();

    // Draw planets with enhancements
    for (size_t i = 0; i < planets.size(); ++i) {
        const Planet& p = planets[i];
        float planetAngle = animationTime * p.orbitSpeed;
        glPushMatrix();

        // Orbit
        glRotatef(planetAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.distance, 0.0f, 0.0f);

        // Self-rotation
        glRotatef(animationTime * p.rotationSpeed, 0.0f, 1.0f, 0.0f);

        // Draw rings if applicable (Saturn)
        if (p.hasRings) {
            glPushMatrix();
            glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Tilt rings
            drawRings(p.size * 1.2f, p.size * 1.5f);
            glPopMatrix();
        }

        // Draw planet
        drawSphere(p.size, p.red, p.green, p.blue);

        // Draw moons
        for (const auto& moon : p.moons) {
            drawMoon(moon, planetAngle);
        }

        glPopMatrix();

        // Draw label (positioned along orbit)
        glPushMatrix();
        glRotatef(planetAngle, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.distance + p.size + 0.5f, 0.0f, 0.0f);
        drawLabel(p.name, 0.0f, 0.0f, 0.0f);
        glPopMatrix();
    }

    // Draw HUD overlay
    drawHUD();

    glutSwapBuffers();
}

// Timer for animation (respects pause)
void timer(int value) {
    if (!paused) {
        animationTime += 1.0f;
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Keyboard for navigation (WASD + arrows for translation, + Z movement)
void keyboard(unsigned char key, int x, int y) {
    float speed = 0.5f;
    switch (key) {
    case 'w': case 'W': cameraY += speed; break;
    case 's': case 'S': cameraY -= speed; break;
    case 'a': case 'A': cameraX -= speed; break;
    case 'd': case 'D': cameraX += speed; break;
    case 'q': case 'Q': cameraZ += speed; break; // Forward
    case 'e': case 'E': cameraZ -= speed; break; // Backward
    case '+': case '=': zoomIn(); break;
    case '-': zoomOut(); break;
    case 'r': case 'R': resetView(); break;
    case 'p': case 'P': togglePause(); break; // Toggle pause with P key too
    case 27: exit(0); break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    float speed = 0.5f;
    switch (key) {
    case GLUT_KEY_UP: cameraY += speed; break;
    case GLUT_KEY_DOWN: cameraY -= speed; break;
    case GLUT_KEY_LEFT: cameraX -= speed; break;
    case GLUT_KEY_RIGHT: cameraX += speed; break;
    }
    glutPostRedisplay();
}

// Mouse controls: Drag for rotation, click for buttons (FIXED: Better hit detection, renamed vars)
void mouse(int button, int state, int x, int y) {
    y = WINDOW_HEIGHT - y; // Flip Y for ortho coords (y=0 at bottom)
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            // Check HUD clicks first (bottom area y < 200)
            bool hudHit = false;
            if (y < 200) {
                for (auto& btn : buttons) {
                    if (x >= btn.x && x <= btn.x + btn.width &&
                        y >= btn.y && y <= btn.y + btn.height) {
                        btn.pressed = true;
                        btn.action(); // Trigger action (now with PostRedisplay inside)
                        hudHit = true;
                        break; // Only one button at a time
                    }
                }
            }
            if (!hudHit) {
                // Outside HUD: Start drag rotation
                mousePressed = true;
                lastMouseX = x;
                lastMouseY = y;
            }
        }
        else {
            // Mouse up: Release
            mousePressed = false;
            for (auto& btn : buttons) {
                btn.pressed = false;
            }
        }
    }
}

void motion(int x, int y) {
    y = WINDOW_HEIGHT - y;
    if (mousePressed) {
        int deltaX = x - lastMouseX;
        int deltaY = y - lastMouseY;
        cameraAngleY += deltaX * 0.5f;
        cameraAngleX += deltaY * 0.5f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

// Reshape (update button positions if window resizes)
void reshape(int w, int h) {
    WINDOW_WIDTH = w;
    WINDOW_HEIGHT = h;
    // Reposition buttons to bottom-right
    for (auto& btn : buttons) {
        btn.x = static_cast<float>(w - 120);
    }
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Init (unchanged, but seed rand for stars)
void init() {
    srand(static_cast<unsigned>(time(nullptr))); // Seed random for star noise
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_position[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Enhanced Solar System with Textured Sun & Star Noise");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
