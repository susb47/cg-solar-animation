#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib> // For rand()
#include <cstdio>  // For sprintf
#include <algorithm> // For std::max, std::min

// M_PI definition for Windows environments if missing
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Window dimensions
int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;

// Animation and camera variables
float animationTime = 0.0f;
float animationSpeed = 1.0f;
float zoom = 1.0f;
float cameraAngleX = 20.0f; // Start with a slight tilt
float cameraAngleY = 0.0f;
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = -40.0f;    // Start zoomed out enough to see the system
bool mousePressed = false;
int lastMouseX = 0, lastMouseY = 0;
bool paused = false;
bool sliderDragging = false;

// HUD Button structure
struct Button {
    float x, y;
    float width, height;
    std::string label;
    bool pressed;
    void (*action)();
};

// Slider structure
struct Slider {
    float x, y;
    float width, height;
    float thumbX;
    std::string label;
    float minVal, maxVal;
    float& value;
};

// Function pointers for buttons
void zoomIn() { cameraZ += 2.0f; glutPostRedisplay(); } // Move camera forward
void zoomOut() { cameraZ -= 2.0f; glutPostRedisplay(); } // Move camera back
void resetView() {
    zoom = 1.0f;
    cameraAngleX = 20.0f;
    cameraAngleY = 0.0f;
    cameraX = cameraY = 0.0f;
    cameraZ = -40.0f;
    animationSpeed = 1.0f; // Reset speed too
    glutPostRedisplay();
}
void togglePause() { paused = !paused; glutPostRedisplay(); }

// Buttons vector
std::vector<Button> buttons = {
    {0, 10.0f, 100.0f, 30.0f, "Zoom In", false, zoomIn},
    {0, 50.0f, 100.0f, 30.0f, "Zoom Out", false, zoomOut},
    {0, 90.0f, 100.0f, 30.0f, "Reset", false, resetView},
    {0, 130.0f, 100.0f, 30.0f, "Pause", false, togglePause}
};

// Speed slider
Slider speedSlider = { 0, 170.0f, 100.0f, 10.0f, 0.333f, "Speed:", 0.0f, 3.0f, animationSpeed };

// Moon structure
struct Moon {
    float distance;
    float size;
    float orbitSpeed;
    float red, green, blue;
};

// Planet structure
struct Planet {
    float distance;
    float size;
    float orbitSpeed;
    float rotationSpeed;
    float axialTilt;
    float red, green, blue;
    std::string name;
    std::vector<Moon> moons;
    bool hasRings;
    int textureType;    // 0: Smooth, 1: Cratered, 2: Banded
};

// Stars vector
std::vector<std::pair<float, float>> stars;

// Planets vector
std::vector<Planet> planets = {
    {2.0f, 0.2f, 4.0f, 2.0f, 0.03f, 0.8f, 0.6f, 0.4f, "Mercury", {}, false, 1},
    {3.0f, 0.3f, 2.5f, -1.5f, 177.4f, 1.0f, 0.8f, 0.4f, "Venus", {}, false, 1},
    {4.5f, 0.3f, 1.8f, 1.0f, 23.5f, 0.2f, 0.5f, 1.0f, "Earth", {{0.8f, 0.05f, 0.5f, 0.5f, 0.8f, 1.0f}}, false, 1},
    {5.5f, 0.25f, 1.2f, 0.8f, 25.2f, 1.0f, 0.4f, 0.2f, "Mars", {}, false, 1},
    {8.0f, 0.8f, 0.6f, 0.5f, 3.1f, 1.0f, 0.7f, 0.3f, "Jupiter", {{1.2f, 0.1f, 1.0f, 0.8f, 0.8f, 0.7f}, {1.5f, 0.08f, 0.9f, 0.7f, 0.6f, 0.5f}}, false, 2},
    {10.0f, 0.7f, 0.4f, 0.4f, 26.7f, 0.9f, 0.8f, 0.5f, "Saturn", {{1.0f, 0.06f, 0.8f, 0.8f, 0.6f, 0.4f}}, true, 2},
    {12.0f, 0.5f, 0.3f, 0.3f, 97.8f, 0.4f, 0.8f, 1.0f, "Uranus", {}, false, 2},
    {14.0f, 0.5f, 0.2f, 0.2f, 28.3f, 0.2f, 0.4f, 1.0f, "Neptune", {}, false, 2}
};

// --- Drawing Functions ---

void drawSmoothSphere(float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glutSolidSphere(radius, 20, 20);
}

void drawCrateredSphere(float radius, float r, float g, float b, int numCraters = 8) {
    drawSmoothSphere(radius, r, g, b);
    // Craters
    for (int i = 0; i < numCraters; ++i) {
        glPushMatrix();
        float theta = (rand() % 360) * M_PI / 180.0f;
        float phi = (rand() % 180) * M_PI / 180.0f;
        float craterRadius = radius * 0.1f;
        float x = radius * sin(phi) * cos(theta);
        float y = radius * cos(phi);
        float z = radius * sin(phi) * sin(theta);
        glTranslatef(x, y, z);
        glColor3f(r * 0.4f, g * 0.4f, b * 0.4f);
        glutSolidSphere(craterRadius, 6, 6);
        glPopMatrix();
    }
}

void drawBandedSphere(float radius, float r, float g, float b, int numBands = 8) {
    // Draw base
    drawSmoothSphere(radius * 0.95f, r, g, b);
    // Draw bands
    for (int i = 0; i < numBands; ++i) {
        glPushMatrix();
        float y = (2.0f * radius / numBands) * i - radius;
        glTranslatef(0.0f, y, 0.0f);
        glRotatef(90, 1, 0, 0); // Rotate to make torus flat
        float colorVar = (i % 2 == 0) ? 0.1f : -0.1f;
        glColor3f(r + colorVar, g + colorVar, b + colorVar);
        glutSolidTorus(radius * 0.05f, radius * 0.9f * cos(asin(y / radius)), 10, 30);
        glPopMatrix();
    }
}

void drawPlanet(const Planet& p) {
    if (p.textureType == 0) drawSmoothSphere(p.size, p.red, p.green, p.blue);
    else if (p.textureType == 1) drawCrateredSphere(p.size, p.red, p.green, p.blue);
    else if (p.textureType == 2) drawBandedSphere(p.size, p.red, p.green, p.blue);
}

void drawSun() {
    glPushMatrix();
    // Core
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.8f, 30, 30);
    // Glow (Rays)
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    for (int i = 0; i < 12; i++) {
        glPushMatrix();
        glRotatef(i * 30 + animationTime, 0, 0, 1);
        glRotatef(animationTime * 2.0f, 0, 1, 0);
        glBegin(GL_TRIANGLES);
        glColor4f(1.0f, 0.5f, 0.0f, 0.6f);
        glVertex3f(0.0f, 0.8f, 0.0f);
        glVertex3f(-0.2f, 0.8f, 0.0f);
        glColor4f(1.0f, 1.0f, 0.0f, 0.0f);
        glVertex3f(0.0f, 2.5f + sin(animationTime * 0.1f), 0.0f);
        glEnd();
        glPopMatrix();
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float angle = 2.0f * M_PI * i / 100.0f;
        glVertex3f(radius * cos(angle), 0.0f, radius * sin(angle));
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawRings(float inner, float outer) {
    glDisable(GL_LIGHTING); // Rings shouldn't be shaded like spheres
    glColor4f(0.8f, 0.8f, 0.7f, 0.8f);
    for (int i = 0; i < 60; ++i) {
        float angle1 = 2.0f * M_PI * i / 60.0f;
        float angle2 = 2.0f * M_PI * (i + 1) / 60.0f;
        glBegin(GL_QUADS);
        glVertex3f(inner * cos(angle1), 0.0f, inner * sin(angle1));
        glVertex3f(outer * cos(angle1), 0.0f, outer * sin(angle1));
        glVertex3f(outer * cos(angle2), 0.0f, outer * sin(angle2));
        glVertex3f(inner * cos(angle2), 0.0f, inner * sin(angle2));
        glEnd();
    }
    glEnable(GL_LIGHTING);
}

void drawStars() {
    glDisable(GL_LIGHTING);
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    for (const auto& star : stars) {
        float twinkle = 0.5f + 0.5f * sin(animationTime + star.first);
        glColor3f(twinkle, twinkle, 1.0f);
        // Draw star far away in background, relative to camera position to create "skybox" effect
        glVertex3f(star.first, star.second, -50.0f); // Draw on a plane behind
        // Also draw some "3D" stars
        glVertex3f(star.first, star.second + 20.0f, star.first * 0.5f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawLabel(const std::string& name) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos3f(0.0f, 0.5f, 0.0f);
    for (char c : name) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }
    glEnable(GL_LIGHTING);
}

void drawSlider(const Slider& slider) {
    // Label
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(slider.x, slider.y + slider.height + 5);
    for (char c : slider.label) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    // Track
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(slider.x, slider.y);
    glVertex2f(slider.x + slider.width, slider.y);
    glVertex2f(slider.x + slider.width, slider.y + slider.height);
    glVertex2f(slider.x, slider.y + slider.height);
    glEnd();

    // Fill
    float thumbPos = slider.x + (slider.thumbX * slider.width);
    glColor3f(0.3f, 0.6f, 0.9f);
    glBegin(GL_QUADS);
    glVertex2f(slider.x, slider.y);
    glVertex2f(thumbPos, slider.y);
    glVertex2f(thumbPos, slider.y + slider.height);
    glVertex2f(slider.x, slider.y + slider.height);
    glEnd();

    // Thumb
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2f(thumbPos - 3, slider.y - 2);
    glVertex2f(thumbPos + 3, slider.y - 2);
    glVertex2f(thumbPos + 3, slider.y + slider.height + 2);
    glVertex2f(thumbPos - 3, slider.y + slider.height + 2);
    glEnd();

    // Value text
    char valStr[10];
    sprintf(valStr, "%.1fx", slider.value);
    glRasterPos2f(slider.x + slider.width + 5, slider.y + 2);
    for (int i = 0; valStr[i] != '\0'; ++i) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, valStr[i]);
}

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

    // Buttons
    for (auto& btn : buttons) {
        if (btn.pressed) glColor3f(0.5f, 0.5f, 0.5f);
        else glColor3f(0.2f, 0.2f, 0.2f);

        glBegin(GL_QUADS);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.width, btn.y);
        glVertex2f(btn.x + btn.width, btn.y + btn.height);
        glVertex2f(btn.x, btn.y + btn.height);
        glEnd();

        glColor3f(0.6f, 0.6f, 0.6f); // Border
        glBegin(GL_LINE_LOOP);
        glVertex2f(btn.x, btn.y);
        glVertex2f(btn.x + btn.width, btn.y);
        glVertex2f(btn.x + btn.width, btn.y + btn.height);
        glVertex2f(btn.x, btn.y + btn.height);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(btn.x + 10, btn.y + 10);
        for (char c : btn.label) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }

    // Slider
    drawSlider(speedSlider);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// --- Display & Logic ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Camera setup (Perspective Logic)
    // Translate the world away from camera (simulating camera moving back)
    glTranslatef(cameraX, cameraY, cameraZ);
    // Rotate the world based on mouse input
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);
    glScalef(zoom, zoom, zoom);

    // Background
    drawStars();

    // Solar System
    drawSun();

    for (const auto& p : planets) {
        drawOrbit(p.distance);

        glPushMatrix();
        // Orbital rotation
        float currentOrbit = animationTime * p.orbitSpeed;
        glRotatef(currentOrbit, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.distance, 0.0f, 0.0f);

        // Label
        glPushMatrix();
        // Counter-rotate label so it faces screen roughly
        glRotatef(-currentOrbit, 0.0f, 1.0f, 0.0f);
        glRotatef(-cameraAngleY, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.0f, p.size + 0.5f, 0.0f);
        drawLabel(p.name);
        glPopMatrix();

        // Axial Tilt & Self Rotation
        glRotatef(p.axialTilt, 1.0f, 0.0f, 0.0f); // Tilt first
        glRotatef(animationTime * p.rotationSpeed, 0.0f, 1.0f, 0.0f); // Spin on axis

        if (p.hasRings) drawRings(p.size * 1.2f, p.size * 2.0f);
        drawPlanet(p);

        // Moons
        for (const auto& m : p.moons) {
            glPushMatrix();
            glRotatef(animationTime * m.orbitSpeed, 0.0f, 1.0f, 0.0f);
            glTranslatef(m.distance, 0.0f, 0.0f);
            drawSmoothSphere(m.size, m.red, m.green, m.blue);
            glPopMatrix();
        }

        glPopMatrix();
    }

    drawHUD();
    glutSwapBuffers();
}

// THIS IS THE FIXED RESHAPE FUNCTION
void reshape(int w, int h) {
    WINDOW_WIDTH = w;
    WINDOW_HEIGHT = h;

    // Update UI positions to stay bottom-right
    for (auto& btn : buttons) btn.x = static_cast<float>(w - 120);
    speedSlider.x = static_cast<float>(w - 120);

    if (h == 0) h = 1;
    float ratio = static_cast<float>(w) / static_cast<float>(h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    // Perspective view: 45 deg FOV, ratio, Near Clip 0.1, Far Clip 500
    gluPerspective(45.0f, ratio, 0.1f, 500.0f);

    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
}

void timer(int value) {
    if (!paused) animationTime += animationSpeed;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void mouse(int button, int state, int x, int y) {
    int glY = WINDOW_HEIGHT - y; // Convert to GL coordinates

    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            bool hudHit = false;
            // HUD Interaction (Buttons)
            if (glY < 250) { // Check lower area
                for (auto& btn : buttons) {
                    if (x >= btn.x && x <= btn.x + btn.width && glY >= btn.y && glY <= btn.y + btn.height) {
                        btn.pressed = true;
                        btn.action();
                        hudHit = true;
                    }
                }
                // Slider Interaction
                float sliderFullHeight = speedSlider.height + 10;
                if (x >= speedSlider.x && x <= speedSlider.x + speedSlider.width &&
                    glY >= speedSlider.y && glY <= speedSlider.y + sliderFullHeight) {
                    sliderDragging = true;
                    hudHit = true;
                    // Initial snap
                    float normX = (float)(x - speedSlider.x) / speedSlider.width;
                    speedSlider.thumbX = std::max(0.0f, std::min(1.0f, normX));
                    speedSlider.value = speedSlider.minVal + speedSlider.thumbX * (speedSlider.maxVal - speedSlider.minVal);
                }
            }

            if (!hudHit) {
                mousePressed = true;
                lastMouseX = x;
                lastMouseY = y;
            }
        }
        else {
            // Mouse Up
            mousePressed = false;
            sliderDragging = false;
            for (auto& btn : buttons) btn.pressed = false;
        }
    }
    // Mouse Wheel (Zoom)
    if (button == 3) zoomIn(); // Scroll Up
    if (button == 4) zoomOut(); // Scroll Down
}

void motion(int x, int y) {
    int glY = WINDOW_HEIGHT - y;

    if (sliderDragging) {
        float normX = (float)(x - speedSlider.x) / speedSlider.width;
        speedSlider.thumbX = std::max(0.0f, std::min(1.0f, normX));
        speedSlider.value = speedSlider.minVal + speedSlider.thumbX * (speedSlider.maxVal - speedSlider.minVal);
        glutPostRedisplay();
    }
    else if (mousePressed) {
        int dx = x - lastMouseX;
        int dy = y - lastMouseY;
        cameraAngleY += dx * 0.5f;
        cameraAngleX += dy * 0.5f;
        lastMouseX = x;
        lastMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': cameraY -= 1.0f; break; // Pan Up
    case 's': cameraY += 1.0f; break; // Pan Down
    case 'a': cameraX += 1.0f; break; // Pan Left
    case 'd': cameraX -= 1.0f; break; // Pan Right
    case 'q': zoomIn(); break;
    case 'e': zoomOut(); break;
    case 'p': togglePause(); break;
    case 'r': resetView(); break;
    case 27: exit(0); break;
    }
    glutPostRedisplay();
}

void init() {
    srand(static_cast<unsigned>(time(nullptr)));

    // Generate Stars
    for (int i = 0; i < 200; i++) {
        float sx = (rand() % 200) - 100.0f;
        float sy = (rand() % 200) - 100.0f;
        stars.push_back({ sx, sy });
    }

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f); // Dark blue space background
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Sun is the light source
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    GLfloat ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    GLfloat diffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Solar System - Corrected Zoom");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
