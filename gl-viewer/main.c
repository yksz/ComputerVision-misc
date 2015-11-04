#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#if defined __APPLE__ && defined __MACH__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif // __APPLE__ && __MACH__
#include "logger.h"

#define PERSPECTIVE_ENABLED 1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif // M_PI

typedef struct
{
    double ex, ey, ez; // 視点位置
    double cx, cy, cz; // 注視点
    double ux, uy, uz; // 上方向ベクトル
} Viewpoint;

typedef struct
{
    GLfloat position[4];  // ライトの位置
    GLfloat ambient[4];   // 環境光
    GLfloat diffuse[4];   // 拡散光（照明のカラー）
    GLfloat specular[4];  // 鏡面光
    GLfloat direction[3]; // スポットの方向
} Light;

typedef struct
{
    GLfloat ambient[4];   // 材質の環境光
    GLfloat diffuse[4];   // 材質の拡散光
    GLfloat specular[4];  // 材質の鏡面光
    GLfloat shininess[1]; // 鏡面係数
} Material;

typedef struct
{
    bool pressed;
    int x, y;
} MouseButton;

static Viewpoint viewpoint = {
    .ex =   0.0, .ey =   0.0, .ez = 200.0,
    .cx =   0.0, .cy =   0.0, .cz =   0.0,
    .ux =   0.0, .uy =   1.0, .uz =   0.0,
};
static Light light0 = {
    .position  = { 50.0f, 100.0f, 50.0f, 1.0f },
    .ambient   = {  0.2f,   0.2f,  0.2f, 1.0f },
    .diffuse   = {  1.0f,   1.0f,  1.0f, 1.0f },
    .specular  = {  1.0f,   1.0f,  1.0f, 1.0f },
    .direction = { -0.5f,  -1.0f, -0.5f },
};
static Material material = {
    .ambient   = {  0.2f, 0.2f, 0.2f, 1.0 },
    .diffuse   = {  1.0f, 0.0f, 0.0f, 1.0 },
    .specular  = {  1.0f, 1.0f, 1.0f, 1.0 },
    .shininess = { 30.0f },
};
static MouseButton leftButton, rightButton;

static void setViewpoint(Viewpoint* v)
{
    gluLookAt(v->ex, v->ey, v->ez,
              v->cx, v->cy, v->cz,
              v->ux, v->uy, v->uz);
}

static void setLight0(Light* l)
{
    glLightfv(GL_LIGHT0, GL_POSITION,       l->position);
    glLightfv(GL_LIGHT0, GL_AMBIENT,        l->ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,        l->diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR,       l->specular);
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, l->direction);
}

static void setMaterial(Material* m)
{
    glMaterialfv(GL_FRONT, GL_AMBIENT,   m->ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   m->diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  m->specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, m->shininess);
}

static void setUpView(int width, int height)
{
    glMatrixMode(GL_PROJECTION); // 現在の行列を射影変換行列に変更する
    glLoadIdentity(); // 変換行列を単位行列に初期化する

    glViewport(0, 0, width, height); // ビューポートの設定

    const double near = 1.0;    // 前方クリップ面と視点間の距離
    const double far  = 1000.0; // 後方クリップ面と視点間の距離
#if PERSPECTIVE_ENABLED
    // 透視投影
    gluPerspective(60.0, // x-z平面の視野角
            (double) width / height, // 視野角の縦横比
            near, far);
#else
    // 正射影
    glOrtho(-0.2 * width, 0.2 * width, -0.2 * height, 0.2 * height, near, far);
#endif // PERSPECTIVE_ENABLED
}

static void rotate(Viewpoint* v, double theta, double phi)
{
    double x = v->ex - v->cx;
    double y = v->ey - v->cy;
    double z = v->ez - v->cz;
    double d = sqrt(x * x + y * y + z * z);
    theta += atan2(x, z);
    phi   += asin(y / d);
    phi = fmax(fmin(phi, M_PI * 0.5), -M_PI * 0.5); // -PI/2 <= phi <= +PI/2
    v->ex = d * sin(theta) * cos(phi) + v->cx;
    v->ey = d * sin(phi)              + v->cy;
    v->ez = d * cos(theta) * cos(phi) + v->cz;
}

static void zoom(Viewpoint* v, double magnification)
{
    double rate = (magnification <= 0) ? 0 : 1 / magnification;
    v->ex = rate * (v->ex - v->cx) + v->cx;
    v->ey = rate * (v->ey - v->cy) + v->cy;
    v->ez = rate * (v->ez - v->cz) + v->cz;
}

static void drawAxes(int len)
{
    glBegin(GL_LINES);

    // x-axis
    glColor3ub(255,   0,   0); // red
    glVertex3i(  0,   0,   0);
    glVertex3i(len,   0,   0);

    // y-axis
    glColor3ub(  0, 255,   0); // green
    glVertex3i(  0,   0,   0);
    glVertex3i(  0, len,   0);

    // z-axis
    glColor3ub(  0,   0, 255); // blue
    glVertex3i(  0,   0,   0);
    glVertex3i(  0,   0, len);

    glEnd();
}

static void idle(void)
{
    glutPostRedisplay();
}

static void display(void)
{
    glMatrixMode(GL_MODELVIEW); // 現在の行列をモデルビュー変換行列に変更する
    glLoadIdentity(); // 変換行列を単位行列に初期化する

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // バッファをクリア

    setViewpoint(&viewpoint);
    setLight0(&light0);
    setMaterial(&material);

    drawAxes(100);
    glutSolidTeapot(50);

    glutSwapBuffers(); // ダブルバッファリングのためのバッファの交換
}

static void reshape(int width, int height)
{
    LOGGER_DEBUG("width=%4d, height=%4d\n", width, height);

    setUpView(width, height);
}

static void keyboard(unsigned char key, int x, int y)
{
    LOGGER_DEBUG("key=%c, x=%4d, y=%4d\n", key, x, y);

    switch (key) {
        case 'q':
            exit(EXIT_SUCCESS);
            break;

        case ' ':
            glutFullScreen();
            break;
    }
}

// click
static void mouse(int button, int state, int x, int y)
{
    LOGGER_DEBUG("button=%d, state=%d, x=%d, y=%d\n", button, state, x, y);

    leftButton.pressed = false;
    rightButton.pressed = false;

    if (state == GLUT_DOWN) {
        switch (button) {
            case GLUT_LEFT_BUTTON:
                leftButton.pressed = true;
                leftButton.x = x;
                leftButton.y = y;
                break;

            case GLUT_RIGHT_BUTTON:
                rightButton.pressed = true;
                rightButton.x = x;
                rightButton.y = y;
                break;
        }
    }
}

// drag
static void motion(int x, int y)
{
    LOGGER_DEBUG("x=%d, y=%d\n", x, y);

    if (leftButton.pressed) {
        const double rotateRate = 0.5;
        double theta = rotateRate * (leftButton.x - x) * M_PI / 180.0;
        double phi   = rotateRate * (y - leftButton.y) * M_PI / 180.0;
        rotate(&viewpoint, theta, phi);
        leftButton.x = x;
        leftButton.y = y;
    } else if (rightButton.pressed) {
        const double zoomRate = 0.01;
        double magnification = 1 + zoomRate * (rightButton.y - y);
        zoom(&viewpoint, magnification);
        rightButton.x = x;
        rightButton.y = y;
    }
}

static void init(void)
{
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // カラーバッファのクリア色の指定
    glClearDepth(1.0); // デプスバッファのクリア値の指定
    glEnable(GL_DEPTH_TEST); // 隠面消去を有効にする
    glEnable(GL_CULL_FACE); // カリングを有効にする
    glCullFace(GL_FRONT); // 裏面のみを描画する
//    glShadeModel(GL_FLAT); // フラットシェーディングを適用する
    glEnable(GL_LIGHTING); // 光源を有効にする
    glEnable(GL_LIGHT0); // ライト0を有効にする
}

int main(int argc, char** argv)
{
    logger_setLevel(LogLevel_DEBUG);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(512, 512);
    glutCreateWindow(argv[0]);

    init();

    glutIdleFunc(idle);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    return 0;
}
