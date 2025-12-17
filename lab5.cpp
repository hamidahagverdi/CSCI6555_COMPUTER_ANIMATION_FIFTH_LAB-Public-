#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <windows.h>
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib> 

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ==========================
// Fancy Lab 5
// Arc-length spline + follow camera + banking + HUD
// ==========================

// ---------- Vec3 ----------
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

static Vec3 operator+(const Vec3& a,const Vec3& b){ return Vec3(a.x+b.x,a.y+b.y,a.z+b.z); }
static Vec3 operator-(const Vec3& a,const Vec3& b){ return Vec3(a.x-b.x,a.y-b.y,a.z-b.z); }
static Vec3 operator*(const Vec3& a,float s){ return Vec3(a.x*s,a.y*s,a.z*s); }
static Vec3 operator/(const Vec3& a,float s){ return Vec3(a.x/s,a.y/s,a.z/s); }

static Vec3 operator-(const Vec3& v){
    return Vec3(-v.x, -v.y, -v.z);
}

static float dot(const Vec3& a,const Vec3& b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static Vec3 cross(const Vec3& a,const Vec3& b){
    return Vec3(a.y*b.z - a.z*b.y,
                a.z*b.x - a.x*b.z,
                a.x*b.y - a.y*b.x);
}
static float length(const Vec3& v){ return std::sqrt(dot(v,v)); }
static Vec3 normalize(const Vec3& v){
    float L = length(v);
    if(L < 1e-6f) return Vec3(0,0,0);
    return v / L;
}
static float clampf(float v,float lo,float hi){
    return std::max(lo,std::min(hi,v));
}

// ---------- Globals ----------
int W = 900, H = 650;
bool paused = false;
bool showPath = true;
bool showGround = true;

int splineMode = 1; // 1=Catmull-Rom, 2=B-spline
float speed = 4.5f;
float sTravel = 0.0f;
float dt = 1.0f / 60.0f;

// Camera (spring)
Vec3 camPos(14,10,18), camVel(0,0,0);
float camStiff = 8.0f, camDamp = 2.2f;

// Object state
Vec3 objPos, objTan, objUp(0,1,0);

// Control points (closed loop)
std::vector<Vec3> P = {
    {-10,3,-10}, {-3,6,-14}, {6,4,-12}, {12,5,-4},
    {10,4,6}, {2,7,12}, {-7,4,9}, {-13,3,0}
};

// Arc-length table
struct Sample { float s; int seg; float u; };
std::vector<Sample> table;
float totalLength = 0.0f;
int samplesPerSeg = 200;

// ---------- Spline math ----------
int wrap(int i,int n){ i%=n; return i<0?i+n:i; }

Vec3 catmullRom(const Vec3& p0,const Vec3& p1,const Vec3& p2,const Vec3& p3,float u){
    float u2=u*u,u3=u2*u;
    return (p1*2 + (p2-p0)*u + (p0*2 - p1*5 + p2*4 - p3)*u2
            + (-p0 + p1*3 - p2*3 + p3)*u3) * 0.5f;
}

Vec3 catmullRomT(const Vec3& p0,const Vec3& p1,const Vec3& p2,const Vec3& p3,float u){
    float u2=u*u;
    return ((p2-p0) + (p0*4 - p1*10 + p2*8 - p3*2)*u
            + (-p0*3 + p1*9 - p2*9 + p3*3)*u2) * 0.5f;
}

Vec3 bSpline(const Vec3& p0,const Vec3& p1,const Vec3& p2,const Vec3& p3,float u){
    float u2=u*u,u3=u2*u;
    float b0=(-u3+3*u2-3*u+1)/6;
    float b1=(3*u3-6*u2+4)/6;
    float b2=(-3*u3+3*u2+3*u+1)/6;
    float b3=u3/6;
    return p0*b0+p1*b1+p2*b2+p3*b3;
}

Vec3 bSplineT(const Vec3& p0,const Vec3& p1,const Vec3& p2,const Vec3& p3,float u){
    float u2=u*u;
    return p0*((-3*u2+6*u-3)/6)
         + p1*((9*u2-12*u)/6)
         + p2*((-9*u2+6*u+3)/6)
         + p3*((3*u2)/6);
}

void evalSpline(int seg,float u,Vec3& pos,Vec3& tan){
    int n=P.size();
    Vec3 p0=P[wrap(seg-1,n)], p1=P[wrap(seg,n)],
         p2=P[wrap(seg+1,n)], p3=P[wrap(seg+2,n)];
    if(splineMode==1){
        pos=catmullRom(p0,p1,p2,p3,u);
        tan=catmullRomT(p0,p1,p2,p3,u);
    }else{
        pos=bSpline(p0,p1,p2,p3,u);
        tan=bSplineT(p0,p1,p2,p3,u);
    }
}

// ---------- Arc length ----------
void buildArc(){
    table.clear();
    totalLength=0;
    Vec3 prev,t;
    evalSpline(0,0,prev,t);
    table.push_back({0,0,0});
    for(int s=0;s<P.size();s++){
        for(int i=1;i<=samplesPerSeg;i++){
            float u=i/(float)samplesPerSeg;
            Vec3 p;
            evalSpline(s,u,p,t);
            totalLength+=length(p-prev);
            table.push_back({totalLength,s,u});
            prev=p;
        }
    }
}

void lookup(float s,int& seg,float& u){
    s=fmod(s,totalLength); if(s<0)s+=totalLength;
    int lo=0,hi=table.size()-1;
    while(lo<hi){
        int m=(lo+hi)/2;
        if(table[m].s<s) lo=m+1; else hi=m;
    }
    if(lo==0){ seg=0; u=0; return; }
    Sample a=table[lo-1], b=table[lo];
    float t=(s-a.s)/(b.s-a.s);
    seg=b.seg; u=a.u+(b.u-a.u)*t;
}

// ---------- Drawing ----------
void drawGround(){
    if(!showGround) return;
    glDisable(GL_LIGHTING);
    glColor3f(0.15f,0.15f,0.2f);
    glBegin(GL_LINES);
    for(int i=-25;i<=25;i++){
        glVertex3f(i,0,-25); glVertex3f(i,0,25);
        glVertex3f(-25,0,i); glVertex3f(25,0,i);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawPath(){
    if(!showPath) return;
    glDisable(GL_LIGHTING);
    glColor3f(1,0.8f,0.2f);
    glBegin(GL_LINE_STRIP);
    for(int s=0;s<P.size();s++)
        for(int i=0;i<=60;i++){
            Vec3 p,t;
            evalSpline(s,i/60.0f,p,t);
            glVertex3f(p.x,p.y,p.z);
        }
    glEnd();
    glEnable(GL_LIGHTING);
}

void drawBoid(){
    glColor3f(0.2f,0.4f,0.9f);
    glutSolidSphere(0.5,20,20);
    glPushMatrix();
    glTranslatef(0,0,0.9f);
    glutSolidCone(0.25,0.7,16,16);
    glPopMatrix();
}

// ---------- Simulation ----------
void step(){
    if(paused) return;
    sTravel+=speed*dt;
    int seg; float u;
    lookup(sTravel,seg,u);
    evalSpline(seg,u,objPos,objTan);

    Vec3 f=normalize(objTan);
    Vec3 desiredCam=objPos - f*10 + Vec3(0,4,0);
    Vec3 x=camPos-desiredCam;
    Vec3 a=x*(-camStiff)-camVel*camDamp;
    camVel=camVel+a*dt;
    camPos=camPos+camVel*dt;
}

// ---------- GLUT ----------
void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(camPos.x,camPos.y,camPos.z,
              objPos.x,objPos.y,objPos.z,0,1,0);

    drawGround();
    drawPath();

    Vec3 f=normalize(objTan);
    Vec3 r=normalize(cross(Vec3(0,1,0),f));
    Vec3 u=cross(f,r);

    float M[16]={
        r.x,u.x,f.x,0,
        r.y,u.y,f.y,0,
        r.z,u.z,f.z,0,
        objPos.x,objPos.y,objPos.z,1
    };

    glPushMatrix();
    glMultMatrixf(M);
    drawBoid();
    glPopMatrix();

    glutSwapBuffers();
}

void reshape(int w,int h){
    W=w;H=h;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60,(float)w/h,0.1,500);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int){
    step();
    glutPostRedisplay();
    glutTimerFunc(16,timer,0);
}

void keyboard(unsigned char k,int,int){
    if(k==27) std::exit(0);
    if(k==' ') paused=!paused;
    if(k=='1'){ splineMode=1; buildArc(); }
    if(k=='2'){ splineMode=2; buildArc(); }
    if(k=='+') speed+=0.5f;
    if(k=='-') speed-=0.5f;
    if(k=='p'||k=='P') showPath=!showPath;
    if(k=='g'||k=='G') showGround=!showGround;
}

void init(){
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f,0.05f,0.08f,1);
    buildArc();
}

int main(int argc,char** argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(W,H);
    glutCreateWindow("Lab 5 Fancy – Arc Length Spline");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0,timer,0);
    glutMainLoop();
    return 0;
}
