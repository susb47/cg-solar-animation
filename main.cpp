#include <GL/glut.h>
#include <cmath>
#include <vector>

// Window dimensions
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Animation variables
float animationTime = 0.0f;
float zoom = 1.0f; // Zoom factor (1.0 = normal, >1 zoom in, <1 zoom out)

// Planet structure
struct Planet {
    float distance;     // Distance from sun
    float size;         // Radius
    float orbitSpeed;   // Orbital speed (degrees per frame)
    float rotationSpeed; // Self-rotation speed (degrees per frame)
    float red, green, blue; // Color
};

// Vector of planets (Sun is separate)
std::vector<Planet> planets = {
    // Mercury
    {2.0f, 0.2f, 4.0f, 2.0f, 0.8f, 0.6f, 0.4f},
    // Venus
    {3.0f, 0.3f, 2.5f, 1.5f, 1.0f, 0.8f, 0.4f},
    // Earth
    {4.5f, 0.3f, 1.8f, 1.0f, 0.2f, 0.5f, 1.0f},
    // Mars
    {5.5f, 0.25f, 1.2f, 0.8f, 1.0f, 0.4f, 0.2f},
    // Jupiter
    {8.0f, 0.8f, 0.6f, 0.5f, 1.0f, 0.7f, 0.3f},
    // Saturn
    {10.0f, 0.7f, 0.4f, 0.4f, 0.9f, 0.8f, 0.5f},
    // Uranus
    {12.0f, 0.5f, 0.3f, 0.3f, 0.4f, 0.8f, 1.0f},
    // Neptune
    {14.0f, 0.5f, 0.2f, 0.2f, 0.2f, 0.4f, 1.0f}
};

// Drawing functions
void drawSphere(float radius, float r, float g, float b) {
    glColor3f(r, g, b);
    glutSolidSphere(radius, 20, 20);
}

void drawSun() {
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow sun
    glutSolidSphere(1.0f, 20, 20);
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Apply zoom (scale the entire scene)
    glScalef(zoom, zoom, zoom);

    // Translate camera back to view the scene
    glTranslatef(0.0f, 0.0f, -20.0f);

    // Draw sun at center
    drawSun();

    // Draw planets
    for (size_t i = 0; i < planets.size(); ++i) {
        const Planet& p = planets[i];
        glPushMatrix();

        // Orbit around sun (rotate on Y axis)
        glRotatef(animationTime * p.orbitSpeed, 0.0f, 1.0f, 0.0f);
        glTranslatef(p.distance, 0.0f, 0.0f);

        // Self-rotation on local Y axis
        glRotatef(animationTime * p.rotationSpeed, 0.0f, 1.0f, 0.0f);

        // Optional: Scale the planet (you can modify this dynamically if needed)
        glScalef(1.0f, 1.0f, 1.0f); // Placeholder for scaling

        // Draw planet
        drawSphere(p.size, p.red, p.green, p.blue);

        glPopMatrix();
    }

    glutSwapBuffers();
}

// Timer callback for animation
void timer(int value) {
    animationTime += 1.0f; // Increment time
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0); // ~60 FPS
}

// Keyboard callback for zoom control
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case '+':
        case '=':
            zoom += 0.1f;
            break;
        case '-':
            zoom -= 0.1f;
            if (zoom < 0.1f) zoom = 0.1f; // Minimum zoom
            break;
        case 'r':
        case 'R':
            zoom = 1.0f; // Reset zoom
            break;
        case 27: // ESC to exit
            exit(0);
            break;
    }
    glutPostRedisplay();
}

// Reshape callback
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / h, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

// Initialization
void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black background
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    // Simple light
    GLfloat light_position[] = {0.0f, 0.0f, 10.0f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
}

// Main function
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("Solar System Simulation");

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}
