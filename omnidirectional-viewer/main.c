#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#if defined __APPLE__ && defined __MACH__
 #include <GLUT/glut.h>
#else
 #include <GL/glut.h>
#endif // __APPLE__ && __MACH__
#include "loader.h"

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif // M_PI

static const unsigned int kTimerPeriod = 100; // [ms]

typedef struct
{
    float ex, ey, ez; // 視点位置
    float cx, cy, cz; // 注視点
    float ux, uy, uz; // 上方向ベクトル
} Viewpoint;

typedef struct
{
    int x, y;
    bool pressed;
} MouseButton;

static Viewpoint viewpoint = {
    .ex = 0.0f, .ey = 0.0f, .ez = 0.0f,
    .cx = 1.0f, .cy = 0.0f, .cz = 0.0f,
    .ux = 0.0f, .uy = 0.0f, .uz = 1.0f,
};
static MouseButton leftButton;

static GLuint displayList;
static GLuint texture;

static void setViewpoint(Viewpoint* v)
{
    gluLookAt(v->ex, v->ey, v->ez,
              v->cx, v->cy, v->cz,
              v->ux, v->uy, v->uz);
}

static void rotate(Viewpoint* v, float theta, float phi)
{
    float x = v->cx - v->ex;
    float y = v->cy - v->ey;
    float z = v->cz - v->ez;
    float d = sqrt(x * x + y * y + z * z);
    theta += atan2(x, y);
    phi += asin(z / d);
    phi = fmaxf(fminf(phi, M_PI * 0.49), -M_PI * 0.49); // -PI/2 <= phi <= +PI/2
    v->cx = d * sin(theta) * cos(phi) + v->ex;
    v->cy = d * cos(theta) * cos(phi) + v->ey;
    v->cz = d * sin(phi)              + v->ez;
}

static void clearBuffer(void)
{
    glClearColor(0.0, 0.0, 0.0, 0.0); // カラーバッファのクリア色の指定
    glClearDepth(1.0); // デプスバッファのクリア値の指定
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //フレームバッファのクリア
}

static void setUpView(int width, int height)
{
    glMatrixMode(GL_PROJECTION); // 現在の行列を射影変換行列に変更する
    glLoadIdentity(); // 変換行列を単位行列に初期化する

    glViewport(0, 0, width, height); // ビューポートの設定

    // 透視投影
    gluPerspective(90.0, // x-z平面の視野角
                   (float) width / height, // 視野角の縦横比
                   1.0,    // 前方クリップ面と視点間の距離
                   100.0); // 後方クリップ面と視点間の距離
}

static void updateTexture(IplImage* img, bool mipmap)
{
    if (mipmap) { // Mipmapの場合は画像サイズの制限はない
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, img->width, img->height,
                GL_BGR_EXT, GL_UNSIGNED_BYTE, img->imageData);
    } else { // 画像サイズは2のn乗でなければならない
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img->width, img->height,
                0, GL_BGR_EXT, GL_UNSIGNED_BYTE, img->imageData);
    }
}

static void callDisplayListWithTexture()
{
    glPushMatrix();
    glEnable(GL_ALPHA_TEST);
    glEnable(GL_TEXTURE_2D);
    glCallList(displayList);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glPopMatrix();
}

static void display()
{
    glMatrixMode(GL_MODELVIEW); // 現在の行列をモデルビュー変換行列に変更する
    glLoadIdentity(); // 変換行列を単位行列に初期化する

    clearBuffer();
    setViewpoint(&viewpoint);

    IplImage* image = Loader_loadImage();
    if (image == NULL) {
        exit(EXIT_FAILURE);
    }
    updateTexture(image, false);
    cvReleaseImage(&image);
    callDisplayListWithTexture();

    glutSwapBuffers(); // ダブルバッファリングのためのバッファの交換
}

static void reshape(int width, int height)
{
    setUpView(width, height);
}

static void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
        case 'q':
            exit(EXIT_SUCCESS);
            break;

        case ' ':
            glutFullScreen();
            break;
    }
}

static void mouseClick(int button, int state, int x, int y)
{
    leftButton.pressed = false;

    if (state == GLUT_DOWN) {
        switch (button) {
            case GLUT_LEFT_BUTTON:
                leftButton.pressed = true;
                leftButton.x = x;
                leftButton.y = y;
                break;

            case GLUT_MIDDLE_BUTTON:
                break;

            case GLUT_RIGHT_BUTTON:
                break;
        }
    }
}

static void mouseDrag(int x, int y)
{
    if (leftButton.pressed) {
        const float rotateRate = 0.2f;
        float theta = rotateRate * (leftButton.x - x) * M_PI / 180.0f;
        float phi   = rotateRate * (y - leftButton.y) * M_PI / 180.0f;
        rotate(&viewpoint, theta, phi);
        leftButton.x = x;
        leftButton.y = y;
    }
}

static void timer(int value)
{
    glutPostRedisplay();
    glutTimerFunc(kTimerPeriod, timer, 0);
}

static void initDisplayList(void)
{
    displayList = glGenLists(1);
    glNewList(displayList, GL_COMPILE);

    GLUquadricObj* sphere = gluNewQuadric();
    gluQuadricNormals(sphere, GLU_SMOOTH); // 面の法線を生成するか、する場合は頂点ごとにひとつの法線を生成するのか、または面ごとに作成するのかを定義
    gluQuadricTexture(sphere, GL_TRUE); // テクスチャ座標をを生成するかを定義
    gluQuadricDrawStyle(sphere, GLU_FILL); // 四辺形を多角形、線、または点の集合として描くかを定義

    // 球を描画(オブジェクト, 半径, 経線方向の分割数, 緯線方向の分割数)
    gluSphere(sphere, 50.0, 32, 32);

    // 円柱の描画(オブジェクト, 底面の半径, 上面の半径, 高さ, 経線方向の分割数, 緯線方向の分割数)
    //gluCylinder(sphere, 2.0, 2.0, 10.0, 64, 64);
    //glTranslatef(0.0, 0.0, -5.0);

    gluDeleteQuadric(sphere);

    glEndList();
}

static void initTexture(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // テクスチャがRGBAで表現されていることをOpenGLに伝える
    glGenTextures(1, &texture); // テクスチャの数を指定
    glBindTexture(GL_TEXTURE_2D, texture); // テクスチャの作成と使用

    // テクスチャを拡大・縮小する方法の指定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // 最近傍法
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // 最近傍法
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // 双線形補間
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 双線形補間

    // テクスチャの繰り返しの指定
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP); // s軸方向の繰り返しを行わない
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP); // t軸方向の繰り返しを行わない
}

static void init(void)
{
    initDisplayList();
    initTexture();
}

static void onExit(void)
{
    glDeleteTextures(1, &texture);
    Loader_free();
}

int main(int argc, char** argv)
{
    if (argc <= 1) {
        fprintf(stderr, "usage: %s [options] <image directory>\n", argv[0]);
        return 1;
    }
    const char* dirname = argv[argc - 1];
    Loader_init(dirname);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(640, 480);
    glutCreateWindow("Omnidirectional Viewer");

    atexit(onExit);
    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouseClick);
    glutMotionFunc(mouseDrag);
    glutTimerFunc(kTimerPeriod, timer, 0);

    glutMainLoop();
    return 0;
}
