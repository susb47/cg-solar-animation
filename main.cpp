#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- 1. ALGORITHM: LINEAR CONGRUENTIAL GENERATOR (LCG) ---
// REASON: Implementing our own Random Number Generator (RNG) instead of using
// the standard rand(). This is how early graphics systems generated noise.
// Formula: X_n+1 = (a * X_n + c) % m
unsigned long seed = 123456789;
float customRand(float min, float max) {
    const unsigned long a = 1103515245;
    const unsigned long c = 12345;
    const unsigned long m = 2147483648;

    seed = (a * seed + c) % m;

    // Normalize to 0.0 - 1.0 range
    float normalized = (float)seed / (float)m;
    return min + normalized * (max - min);
}

// --- CONFIGURATION ---
int WINDOW_WIDTH = 1200;
int WINDOW_HEIGHT = 800;

// Camera & Animation
float animationTime = 0.0f;
float animationSpeed = 1.0f;
float zoom = 1.0f;
float cameraAngleX = 20.0f;
float cameraAngleY = 0.0f;
float cameraX = 0.0f;
float cameraY = 0.0f;
float cameraZ = -40.0f;
bool mousePressed = false;
int lastMouseX = 0, lastMouseY = 0;
bool paused = false;
bool sliderDragging = false;

// --- DATA STRUCTURES ---
struct Button {
    float x, y, width, height;
    std::string label;
    bool pressed;
    void (*action)();
};

struct Slider {
    float x, y, width, height;
    float thumbX;
    std::string label;
    float minVal, maxVal;
    float& value;
};

struct Moon {
    float distance, size, orbitSpeed;
    float red, green, blue;
};

struct Planet {
    float distance, size, orbitSpeed, rotationSpeed, axialTilt;
    float red, green, blue;
    std::string name;
    std::vector<Moon> moons;
    bool hasRings;
    int textureType; // 0:Smooth, 1:Cratered, 2:Banded, 3:Earth
};

// --- CONTROLS CALLBACKS ---
void zoomIn() { cameraZ += 2.0f; glutPostRedisplay(); }
void zoomOut() { cameraZ -= 2.0f; glutPostRedisplay(); }
void resetView() {
    zoom = 1.0f; cameraAngleX = 20.0f; cameraAngleY = 0.0f;
    cameraX = 0.0f; cameraY = 0.0f; cameraZ = -40.0f;
    animationSpeed = 1.0f;
    glutPostRedisplay();
}
void togglePause() { paused = !paused; glutPostRedisplay(); }

// UI Elements
std::vector<Button> buttons = {
    {0, 10, 100, 30, "Zoom In", false, zoomIn},
    {0, 50, 100, 30, "Zoom Out", false, zoomOut},
    {0, 90, 100, 30, "Reset", false, resetView},
    {0, 130, 100, 30, "Pause", false, togglePause}
};

Slider speedSlider = { 0, 170, 100, 10, 0.333f, "Speed:", 0.0f, 3.0f, animationSpeed };

std::vector<std::pair<float, float>> stars;

std::vector<Planet> planets = {
    {2.0f, 0.2f, 4.0f, 2.0f, 0.03f, 0.8f, 0.6f, 0.4f, "Mercury", {}, false, 1},
    {3.0f, 0.3f, 2.5f, -1.5f, 177.4f, 1.0f, 0.8f, 0.4f, "Venus", {}, false, 1},
    {4.5f, 0.3f, 1.8f, 1.0f, 23.5f, 0.2f, 0.5f, 1.0f, "Earth", {{0.8f, 0.05f, 0.5f, 0.5f, 0.8f, 1.0f}}, false, 3},
    {5.5f, 0.25f, 1.2f, 0.8f, 25.2f, 1.0f, 0.4f, 0.2f, "Mars", {}, false, 1},
    {8.0f, 0.8f, 0.6f, 0.5f, 3.1f, 1.0f, 0.7f, 0.3f, "Jupiter", {{1.2f, 0.1f, 1.0f, 0.8f, 0.8f, 0.7f}, {1.5f, 0.08f, 0.9f, 0.7f, 0.6f, 0.5f}}, false, 2},
    {10.0f, 0.7f, 0.4f, 0.4f, 26.7f, 0.9f, 0.8f, 0.5f, "Saturn", {{1.0f, 0.06f, 0.8f, 0.8f, 0.6f, 0.4f}}, true, 2},
    {12.0f, 0.5f, 0.3f, 0.3f, 97.8f, 0.4f, 0.8f, 1.0f, "Uranus", {}, false, 2},
    {14.0f, 0.5f, 0.2f, 0.2f, 28.3f, 0.2f, 0.4f, 1.0f, "Neptune", {}, false, 2}
};

// --- DRAWING HELPERS ---

void drawSurfacePatch(float planetRadius, float patchSize, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(planetRadius, 0.0f, 0.0f);
    glScalef(0.05f, 1.0f, 1.0f);
    glColor3f(r, g, b);
    glutSolidSphere(patchSize, 20, 20);
    glPopMatrix();
}

void drawSmoothSphere(float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glutSolidSphere(radius, 50, 50);
}

void drawCrateredSphere(float radius, float r, float g, float b) {
    drawSmoothSphere(radius, r, g, b);
    // Use custom RNG for crater placement
    // REASON: Ensuring consistent procedural generation using LCG
    seed = (unsigned long)(radius * 1000);
    for (int i = 0; i < 12; ++i) {
        glPushMatrix();
        float theta = customRand(0, 360);
        float phi = customRand(0, 180);
        glRotatef(theta, 0, 1, 0);
        glRotatef(phi, 0, 0, 1);
        float craterSize = radius * (0.15f + customRand(0, 10) / 100.0f);
        drawSurfacePatch(radius, craterSize, r * 0.6f, g * 0.6f, b * 0.6f);
        glPopMatrix();
    }
}

void drawEarth(float radius) {
    drawSmoothSphere(radius, 0.0f, 0.4f, 0.8f);
    struct Land { float lat, lon, size; };
    std::vector<Land> continents = {
        {0.0f, 0.0f, 0.8f}, {45.0f, 90.0f, 0.7f}, {-20.0f, -60.0f, 0.6f},
        {40.0f, -100.0f, 0.7f}, {-45.0f, 130.0f, 0.5f}
    };
    for (const auto& land : continents) {
        glPushMatrix();
        glRotatef(land.lon, 0, 1, 0);
        glRotatef(land.lat, 0, 0, 1);
        drawSurfacePatch(radius, radius * land.size, 0.1f, 0.6f, 0.1f);
        glPopMatrix();
    }
}

// --- 2. ALGORITHM: LAMBERTIAN REFLECTION (Manual Lighting) ---
// REASON: Manually calculating diffuse lighting intensity using the Dot Product.
// Intensity = Normal_Vector dot Light_Vector
// This replaces reliance on OpenGL's state machine for this specific object.
void drawBandedSphere(float radius, float r, float g, float b) {
    int stacks = 40;
    int slices = 40;

    // Light source direction (Coming from origin 0,0,0 where Sun is)
    // In local planet space, the light direction is effectively the inverse of position,
    // but for simplicity here we approximate a directional light from "left" (-X)
    float lightDirX = -1.0f, lightDirY = 0.0f, lightDirZ = 0.0f;

    for (int i = 0; i < stacks; ++i) {
        float lat0 = M_PI * (-0.5f + (float)(i) / stacks);
        float z0 = sin(lat0);
        float zr0 = cos(lat0);
        float lat1 = M_PI * (-0.5f + (float)(i + 1) / stacks);
        float z1 = sin(lat1);
        float zr1 = cos(lat1);

        bool isBand = (i / 4) % 2 == 0;
        float baseR = isBand ? r : r * 0.85f;
        float baseG = isBand ? g : g * 0.85f;
        float baseB = isBand ? b : b * 0.85f;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= slices; ++j) {
            float lng = 2 * M_PI * (float)(j - 1) / slices;
            float x = cos(lng);
            float y = sin(lng);

            // Calculate Surface Normal
            float nx = x * zr0;
            float ny = y * zr0;
            float nz = z0;

            // --- ALGORITHM STEP: Dot Product for Diffuse Intensity ---
            // Dot Product = (nx*lx) + (ny*ly) + (nz*lz)
            // We take max(0.2, dot) to ensure a minimum ambient light of 0.2
            float dotProduct = (nx * lightDirX) + (ny * lightDirY) + (nz * lightDirZ);
            float intensity = std::max(0.2f, dotProduct); // Clamp min brightness

            // Apply calculated color
            glColor3f(baseR * intensity, baseG * intensity, baseB * intensity);

            glNormal3f(nx, ny, nz);
            glVertex3f(radius * nx, radius * ny, radius * nz);

            // Repeat for second vertex in strip
            nx = x * zr1; ny = y * zr1; nz = z1;
            dotProduct = (nx * lightDirX) + (ny * lightDirY) + (nz * lightDirZ);
            intensity = std::max(0.2f, dotProduct);

            glColor3f(baseR * intensity, baseG * intensity, baseB * intensity);
            glNormal3f(nx, ny, nz);
            glVertex3f(radius * nx, radius * ny, radius * nz);
        }
        glEnd();
    }
}

void drawPlanet(const Planet& p) {
    glPushMatrix();
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    if (p.textureType == 0) drawSmoothSphere(p.size, p.red, p.green, p.blue);
    else if (p.textureType == 1) drawCrateredSphere(p.size, p.red, p.green, p.blue);
    else if (p.textureType == 2) drawBandedSphere(p.size, p.red, p.green, p.blue);
    else if (p.textureType == 3) drawEarth(p.size);
    glPopMatrix();
}

void drawSun() {
    glPushMatrix();
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(0.8f, 30, 30);
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

// --- 3. ALGORITHM: MIDPOINT CIRCLE ALGORITHM (Adapted) ---
// REASON: Replaces standard 'cos/sin' loop. The Midpoint algorithm uses integer arithmetic
// to determine pixel positions for a circle. 
// ADAPTATION: Since we are in 3D float space (not 2D pixel space), we scale the radius UP
// to run the integer algorithm, then scale the result DOWN to get smooth float coordinates.
void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_POINTS); // Drawing points to show the algorithm steps clearly

    // Scale up to use integer math (precision preservation)
    int r = (int)(radius * 100.0f);
    int x = 0;
    int y = r;
    int p = 1 - r; // Initial decision parameter

    // Standard Midpoint Circle Algorithm loop
    // Draws 8 octants simultaneously using symmetry
    while (x <= y) {
        float fx = x / 100.0f; // Scale back down to world coordinates
        float fy = y / 100.0f;

        // Plot 8 octants on the XZ plane (y=0)
        glVertex3f(fx, 0, fy);
        glVertex3f(fy, 0, fx);
        glVertex3f(-fx, 0, fy);
        glVertex3f(-fy, 0, fx);
        glVertex3f(-fx, 0, -fy);
        glVertex3f(-fy, 0, -fx);
        glVertex3f(fx, 0, -fy);
        glVertex3f(fy, 0, -fx);

        x++;
        if (p < 0) {
            p += 2 * x + 1;
        }
        else {
            y--;
            p += 2 * (x - y) + 1;
        }
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawRings(float inner, float outer) {
    glDisable(GL_LIGHTING);
    glColor4f(0.8f, 0.8f, 0.7f, 0.7f);
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
        glVertex3f(star.first, star.second, -50.0f);
        glVertex3f(star.first, star.second + 20.0f, star.first * 0.5f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawLabel(const std::string& name) {
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos3f(0.0f, 0.5f, 0.0f);
    for (char c : name) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    glEnable(GL_LIGHTING);
}

// --- HUD & INTERACTION ---

void drawSlider(const Slider& slider) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(slider.x, slider.y + slider.height + 5);
    for (char c : slider.label) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);

    glColor3f(0.3f, 0.3f, 0.3f);
    glRectf(slider.x, slider.y, slider.x + slider.width, slider.y + slider.height);

    float thumbPos = slider.x + (slider.thumbX * slider.width);
    glColor3f(0.3f, 0.6f, 0.9f);
    glRectf(slider.x, slider.y, thumbPos, slider.y + slider.height);

    glColor3f(1.0f, 1.0f, 1.0f);
    glRectf(thumbPos - 3, slider.y - 2, thumbPos + 3, slider.y + slider.height + 2);

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

    for (auto& btn : buttons) {
        if (btn.pressed) glColor3f(0.5f, 0.5f, 0.5f); else glColor3f(0.2f, 0.2f, 0.2f);
        glRectf(btn.x, btn.y, btn.x + btn.width, btn.y + btn.height);

        glColor3f(0.6f, 0.6f, 0.6f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(btn.x, btn.y); glVertex2f(btn.x + btn.width, btn.y);
        glVertex2f(btn.x + btn.width, btn.y + btn.height); glVertex2f(btn.x, btn.y + btn.height);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(btn.x + 10, btn.y + 10);
        for (char c : btn.label) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
    }

    drawSlider(speedSlider);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// --- MAIN LOOP ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glTranslatef(cameraX, cameraY, cameraZ);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);
    glScalef(zoom, zoom, zoom);

    drawStars();
    drawSun();

    for (const auto& p : planets) {
        // Uses Midpoint Circle Algorithm for orbit
        drawOrbit(p.distance);

        glPushMatrix();
        float currentOrbit = animationTime * p.orbitSpeed;
        glRotatef(currentOrbit, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.distance, 0.0f, 0.0f);

        glPushMatrix();
        glRotatef(-currentOrbit, 0.0f, 1.0f, 0.0f);
        glRotatef(-cameraAngleY, 0.0f, 1.0f, 0.0f);
        glTranslatef(0.0f, p.size + 0.5f, 0.0f);
        drawLabel(p.name);
        glPopMatrix();

        glRotatef(p.axialTilt, 1.0f, 0.0f, 0.0f);
        glRotatef(animationTime * p.rotationSpeed, 0.0f, 1.0f, 0.0f);

        if (p.hasRings) drawRings(p.size * 1.2f, p.size * 2.0f);
        drawPlanet(p);

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

void reshape(int w, int h) {
    WINDOW_WIDTH = w;
    WINDOW_HEIGHT = h;
    for (auto& btn : buttons) btn.x = static_cast<float>(w - 120);
    speedSlider.x = static_cast<float>(w - 120);

    if (h == 0) h = 1;
    float ratio = static_cast<float>(w) / static_cast<float>(h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
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
    int glY = WINDOW_HEIGHT - y;
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            bool hudHit = false;
            if (glY < 250) {
                for (auto& btn : buttons) {
                    if (x >= btn.x && x <= btn.x + btn.width && glY >= btn.y && glY <= btn.y + btn.height) {
                        btn.pressed = true; btn.action(); hudHit = true;
                    }
                }
                float sliderH = speedSlider.height + 10;
                if (x >= speedSlider.x && x <= speedSlider.x + speedSlider.width && glY >= speedSlider.y && glY <= speedSlider.y + sliderH) {
                    sliderDragging = true; hudHit = true;
                    float normX = (float)(x - speedSlider.x) / speedSlider.width;
                    speedSlider.thumbX = std::max(0.0f, std::min(1.0f, normX));
                    speedSlider.value = speedSlider.minVal + speedSlider.thumbX * (speedSlider.maxVal - speedSlider.minVal);
                }
            }
            if (!hudHit) { mousePressed = true; lastMouseX = x; lastMouseY = y; }
        }
        else {
            mousePressed = false; sliderDragging = false;
            for (auto& btn : buttons) btn.pressed = false;
        }
    }
    if (button == 3) zoomIn();
    if (button == 4) zoomOut();
}

void motion(int x, int y) {
    if (sliderDragging) {
        float normX = (float)(x - speedSlider.x) / speedSlider.width;
        speedSlider.thumbX = std::max(0.0f, std::min(1.0f, normX));
        speedSlider.value = speedSlider.minVal + speedSlider.thumbX * (speedSlider.maxVal - speedSlider.minVal);
        glutPostRedisplay();
    }
    else if (mousePressed) {
        int dx = x - lastMouseX; int dy = y - lastMouseY;
        cameraAngleY += dx * 0.5f; cameraAngleX += dy * 0.5f;
        lastMouseX = x; lastMouseY = y;
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 'w': cameraY -= 1.0f; break; case 's': cameraY += 1.0f; break;
    case 'a': cameraX += 1.0f; break; case 'd': cameraX -= 1.0f; break;
    case 'q': zoomIn(); break; case 'e': zoomOut(); break;
    case 'p': togglePause(); break; case 'r': resetView(); break;
    case 27: exit(0); break;
    }
    glutPostRedisplay();
}

void init() {
    // Seed our custom LCG Random Generator
    seed = (unsigned long)time(nullptr);

    // Generate Stars using Custom RNG
    for (int i = 0; i < 200; i++) {
        stars.push_back({ customRand(-100, 100), customRand(-100, 100) });
    }

    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Solar System - Algorithms Implemented");
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
