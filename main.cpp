#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <string>

// Window dimensions
const int WINDOW_WIDTH = 1200;
const int WINDOW_HEIGHT = 800;

// Animation and camera variables
float animationTime = 0.0f;
float zoom = 1.0f;
float cameraAngleX = 0.0f; // Mouse rotation on X
float cameraAngleY = 0.0f; // Mouse rotation on Y
bool mousePressed = false;
int mouseX = 0, mouseY = 0;

// Planet structure (enhanced with moons and rings)
struct Planet {
    float distance;     // Distance from sun
    float size;         // Radius
    float orbitSpeed;   // Orbital speed (degrees per frame)
    float rotationSpeed; // Self-rotation speed (degrees per frame)
    float red, green, blue; // Color
    std::string name;   // Name for labeling
    std::vector<struct Moon> moons; // Moons
    bool hasRings;      // For Saturn
};

// Moon structure
struct Moon {
    float distance;     // Distance from planet
    float size;         // Radius
    float orbitSpeed;   // Orbital speed around planet
    float red, green, blue;
};

// Planets vector (enhanced data)
std::vector<Planet> planets = {
    // Mercury
    {2.0f, 0.2f, 4.0f, 2.0f, 0.8f, 0.6f, 0.4f, "Mercury", {}},
    // Venus
    {3.0f, 0.3f, 2.5f, 1.5f, 1.0f, 0.8f, 0.4f, "Venus", {}},
    // Earth (with Moon)
    {4.5f, 0.3f, 1.8f, 1.0f, 0.2f, 0.5f, 1.0f, "Earth",
        {{0.8f, 0.05f, 0.5f, 0.5f, 0.5f, 0.8f, 1.0f}} // Moon
    },
    // Mars
    {5.5f, 0.25f, 1.2f, 0.8f, 1.0f, 0.4f, 0.2f, "Mars", {}},
    // Jupiter (with 2 moons)
    {8.0f, 0.8f, 0.6f, 0.5f, 1.0f, 0.7f, 0.3f, "Jupiter",
        {{1.2f, 0.1f, 1.0f, 0.8f, 0.8f}, {1.5f, 0.08f, 0.9f, 0.7f, 0.6f}}
    },
    // Saturn (with rings and 1 moon)
    {10.0f, 0.7f, 0.4f, 0.4f, 0.9f, 0.8f, 0.5f, "Saturn", {{1.0f, 0.06f, 0.8f, 0.8f, 0.6f}}, true},
    // Uranus
    {12.0f, 0.5f, 0.3f, 0.3f, 0.4f, 0.8f, 1.0f, "Uranus", {}},
    // Neptune
    {14.0f, 0.5f, 0.2f, 0.2f, 0.2f, 0.4f, 1.0f, "Neptune", {}}
};

// Stars for background (simple points)
std::vector<std::pair<float, float>> stars = {
    {-15.0f, -10.0f}, {18.0f, 5.0f}, {-12.0f, 8.0f}, {10.0f, -12.0f}, // Add more as needed
    {5.0f, 15.0f}, {-8.0f, -5.0f}, {20.0f, 2.0f}, {-3.0f, 18.0f},
    {16.0f, -8.0f}, {-10.0f, 12.0f}, {7.0f, -15.0f}, { -16.0f, 6.0f}
};

// Drawing functions
void drawSphere(float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glutSolidSphere(radius, 20, 20);
}

void drawSun() {
    glColor3f(1.0f, 1.0f, 0.0f);
    glutSolidSphere(1.0f, 20, 20);
}

void drawOrbit(float radius) {
    glColor3f(0.3f, 0.3f, 0.3f); // Gray orbit lines
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float angle = 2.0f * M_PI * i / 100.0f;
        glVertex3f(radius * cos(angle), 0.0f, radius * sin(angle));
    }
    glEnd();
}

void drawRings(float innerRadius, float outerRadius) {
    glColor3f(0.8f, 0.8f, 0.7f); // Light gray rings
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

void drawStars() {
    glColor3f(1.0f, 1.0f, 1.0f); // White stars
    glBegin(GL_POINTS);
    glPointSize(2.0f);
    for (const auto& star : stars) {
        glVertex3f(star.first, 5.0f + sin(animationTime * 0.1f), star.second); // Slight twinkle
    }
    glEnd();
}

void drawLabel(const std::string& name, float x, float y, float z) {
    glRasterPos3f(x, y, z);
    for (char c : name) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }
}
// Display callback (enhanced)
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply zoom and camera rotation (mouse controls)
    glScalef(zoom, zoom, zoom);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, -20.0f);

    // Draw background stars
    drawStars();

    // Draw orbital paths
    for (const auto& p : planets) {
        drawOrbit(p.distance);
    }

    // Draw sun
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

    glutSwapBuffers();
}

// Timer for animation
void timer(int value) {
    animationTime += 1.0f;
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// Keyboard for zoom/info
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case '+': case '=': zoom += 0.1f; break;
    case '-': zoom = std::max(0.1f, zoom - 0.1f); break;
    case 'r': case 'R': zoom = 1.0f; cameraAngleX = cameraAngleY = 0.0f; break;
    case 27: exit(0); break;
    }
    glutPostRedisplay();
}

// Mouse controls for camera
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            mousePressed = true;
            mouseX = x; mouseY = y;
        }
        else {
            mousePressed = false;
        }
    }
}

void motion(int x, int y) {
    if (mousePressed) {
        cameraAngleY += (x - mouseX) * 0.5f;
        cameraAngleX += (y - mouseY) * 0.5f;
        mouseX = x; mouseY = y;
        glutPostRedisplay();
    }
}

// Reshape
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Init (enhanced lighting)
void init() {
    glClearColor(0.0f, 0.0f, 0.1f, 1.0f); // Dark blue space
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat light_position[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Sun as light source
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Enhanced Solar System - 3-Person Project");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
