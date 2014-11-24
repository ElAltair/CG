// ===============================================================================
//						  NVIDIA PHYSX SDK TRAINING PROGRAMS
//						     LESSON 101 : PRIMARY SHAPE
//
//						    Written by QA BJ, 6-2-2008
// ===============================================================================

#include "Lesson101.h"
#include "Timing.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <math.h>
//#include "Settings.h"            //Файл конфигов
//#include "AppSettings.h"
#include "tinyxml.h"
#include "tinystr.h"

using namespace std;

int GRAPHPOINTSCOUNTX;    //Число точек по x
int GRAPHPOINTSCOUNTY;   //Число точек по y
int STEP;                //Шаг сетки графа
int OBJECTVELOCITY;      //скорость
int OBJECTSCOUNT;       //количество сфер
int OFFSETX;       //максимальные смещения по х от основы сетки
int OFFSETY;        //максимальные смещения по х от основы сетки
NxVec3 colorMass[1000];

int* globalCount;     //вектор 
int* globalCountofI;  //вектор
int* Pstart;         //вектор начальных положений сфер
int* Pfinish;       // вектор конечных положений сфер
int* flag;          //  ?

NxActor** Spheres; // массив сфер
NxVec3** graphPoints;  //массив точек графа
int** MatrixSmezhnost; //матрица смежности
int*** PointsOfTheWay;  // массив всех возможных точек пути для каждой сферы
int** WayPoints;        //


// Physics SDK globals
NxPhysicsSDK*     gPhysicsSDK = NULL;
NxScene*          gScene = NULL;
NxVec3            gDefaultGravity(0, -9.8, 0);

// User report globals
DebugRenderer     gDebugRenderer;

// HUD globals
HUD hud;

// Display globals
int gMainHandle;
int mx = 0;
int my = 0;

// Camera globals
float	gCameraAspectRatio = 1.0f;
NxVec3	gCameraPos(STEP * GRAPHPOINTSCOUNTX / 2, 50, -STEP * GRAPHPOINTSCOUNTX);
NxVec3	gCameraForward(0, 0, 1);
NxVec3	gCameraRight(-1, 0, 0);
const NxReal gCameraSpeed = 100;

// Force globals
NxVec3	gForceVec(0, 0, 0);
NxReal	gForceStrength = 20000;
bool	bForceMode = true;

// Keyboard globals
#define MAX_KEYS 256
bool gKeys[MAX_KEYS];

// Simulation globals
NxReal	gDeltaTime = 1.0 / 60.0;
bool	bHardwareScene = false;
bool	bPause = false;
bool	bShadows = true;
bool	bDebugWireframeMode = false;

// Actor globals
NxActor* groundPlane = NULL;

// Focus actor
NxActor* gSelectedActor = NULL;

void PrintControls()
{
	printf("\n Flight Controls:\n ----------------\n w = forward, s = back\n a = strafe left, d = strafe right\n q = up, z = down\n");
	printf("\n Force Controls:\n ---------------\n i = +z, k = -z\n j = +x, l = -x\n u = +y, m = -y\n");
	printf("\n Miscellaneous:\n --------------\n p   = Pause\n b   = Toggle Debug Wireframe Mode\n x   = Toggle Shadows\n r   = Select Actor\n F10 = Reset scene\n");
}

NxVec3 ApplyForceToActor(NxActor* actor, const NxVec3& forceDir, const NxReal forceStrength)
{
	NxVec3 forceVec = forceStrength*forceDir*gDeltaTime;
	actor->addForce(forceVec);
	return forceVec;
}

void ProcessCameraKeys()
{
	NxReal deltaTime;

	if (bPause)
	{
		deltaTime = 0.0005;
	}
	else
	{
		deltaTime = gDeltaTime;
	}

	// Process camera keys
	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (!gKeys[i])  { continue; }

		switch (i)
		{
			// Camera controls
		case 'w':{ gCameraPos += gCameraForward*gCameraSpeed*deltaTime; break; }
		case 's':{ gCameraPos -= gCameraForward*gCameraSpeed*deltaTime; break; }
		case 'a':{ gCameraPos -= gCameraRight*gCameraSpeed*deltaTime;	break; }
		case 'd':{ gCameraPos += gCameraRight*gCameraSpeed*deltaTime;	break; }
		case 'z':{ gCameraPos -= NxVec3(0, 1, 0)*gCameraSpeed*deltaTime;	break; }
		case 'q':{ gCameraPos += NxVec3(0, 1, 0)*gCameraSpeed*deltaTime;	break; }
		}
	}
}

void SetupCamera()
{
	gCameraAspectRatio = (float)glutGet(GLUT_WINDOW_WIDTH) / (float)glutGet(GLUT_WINDOW_HEIGHT);

	// Setup camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, gCameraAspectRatio, 1.0f, 10000.0f);
	gluLookAt(gCameraPos.x, gCameraPos.y, gCameraPos.z, gCameraPos.x + gCameraForward.x, gCameraPos.y + gCameraForward.y, gCameraPos.z + gCameraForward.z, 0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RenderActors(bool shadows)
{
	// Render all the actors in the scene
	NxU32 nbActors = gScene->getNbActors();
	NxActor** actors = gScene->getActors();
	while (nbActors--)
	{
		NxActor* actor = *actors++;
		DrawActor(actor, gSelectedActor, false);

		// Handle shadows
		if (shadows)
		{
			DrawActorShadow(actor, false);
		}
	}
}

void DrawForce(NxActor* actor, NxVec3& forceVec, const NxVec3& color)
{
	// Draw only if the force is large enough
	NxReal force = forceVec.magnitude();
	if (force < 0.1)  return;

	forceVec = 3 * forceVec / force;

	NxVec3 pos = actor->getCMassGlobalPosition();
	DrawArrow(pos, pos + forceVec, color);
}

bool IsSelectable(NxActor* actor)
{
	NxShape*const* shapes = gSelectedActor->getShapes();
	NxU32 nShapes = gSelectedActor->getNbShapes();
	while (nShapes--)
	{
		if (shapes[nShapes]->getFlag(NX_TRIGGER_ENABLE))
		{
			return false;
		}
	}

	if (!actor->isDynamic())
		return false;

	if (actor == groundPlane)
		return false;

	return true;
}

void SelectNextActor()
{
	NxU32 nbActors = gScene->getNbActors();
	NxActor** actors = gScene->getActors();
	for (NxU32 i = 0; i < nbActors; i++)
	{
		if (actors[i] == gSelectedActor)
		{
			NxU32 j = 1;
			gSelectedActor = actors[(i + j) % nbActors];
			while (!IsSelectable(gSelectedActor))
			{
				j++;
				gSelectedActor = actors[(i + j) % nbActors];
			}
			break;
		}
	}
}

void ProcessForceKeys()
{
	// Process force keys
	for (int i = 0; i < MAX_KEYS; i++)
	{
		if (!gKeys[i])  { continue; }

		switch (i)
		{
			// Force controls
		case 'i': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(0, 0, 1), gForceStrength);		break; }
		case 'k': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(0, 0, -1), gForceStrength);	break; }
		case 'j': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(1, 0, 0), gForceStrength);		break; }
		case 'l': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(-1, 0, 0), gForceStrength);	break; }
		case 'u': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(0, 1, 0), gForceStrength);		break; }
		case 'm': { gForceVec = ApplyForceToActor(gSelectedActor, NxVec3(0, -1, 0), gForceStrength);	break; }

			// Return box to (0,5,0)
		case 't':
		{
					if (gSelectedActor)
					{
						gSelectedActor->setGlobalPosition(NxVec3(0, 5, 0));
						gScene->flushCaches();
					}
					break;
		}
		}
	}
}

void ProcessInputs()
{
	ProcessForceKeys();

	// Show debug wireframes
	if (bDebugWireframeMode)
	{
		if (gScene)  gDebugRenderer.renderData(*gScene->getDebugRenderable());
	}
}

void masRevers(int** WayPoints, int SphereNumber, int PointIndex)
{
	int vspWayPoints[200];
	for (int i = 0; i <= PointIndex; i++)
		vspWayPoints[i] = WayPoints[SphereNumber][i];
	for (int i = PointIndex, j = 0; i >= 0; i--, j++)
		WayPoints[SphereNumber][j] = vspWayPoints[i];
}

void createPoints()
{
	float x, y, delX(0), delY(0);
	for (int i = 0; i < GRAPHPOINTSCOUNTX; i++)
	for (int j = 0; j < GRAPHPOINTSCOUNTY; j++)
	{
		if (OFFSETX != 0)
			delX = (rand() % OFFSETX);
		if (OFFSETY != 0)
			delY = (rand() % OFFSETY);
		x = i * STEP + delX;
		y = j * STEP + delY;
		graphPoints[i][j] = NxVec3(x, 0, y);
	}
}

void createWay(int SphereNumber, int countOfPoints, int start, int finish)
{
	int PointIndex = 0;
	int PrevPoint = 100000;
	for (int i = 2000; i > 0; i--)
	{
		if (PointsOfTheWay[SphereNumber][i][1] == finish)
		{
			WayPoints[SphereNumber][PointIndex] = finish;
			PointIndex++;
			PrevPoint = PointsOfTheWay[SphereNumber][i][0];
			WayPoints[SphereNumber][PointIndex] = PrevPoint;
			PointIndex++;
		}
		else
		if (PointsOfTheWay[SphereNumber][i][1] == PrevPoint)
		{
			PrevPoint = PointsOfTheWay[SphereNumber][i][0];
			WayPoints[SphereNumber][PointIndex] = PrevPoint;
			if (PrevPoint == start)
				break;
			else
				PointIndex++;
		}
	}
	WayPoints[SphereNumber][PointIndex] = start;
	masRevers(WayPoints, SphereNumber, PointIndex);
}

void Dejkstra(int SphereNumber, int start, int finish)
{
	for (int j = 0; j < 200; j++)
		WayPoints[SphereNumber][j] = INT_MAX;
	int* distance, count, index, i, u, m = start + 1;
	distance = new int[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY];
	bool* visited;
	visited = new bool[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY];
	for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
	{
		distance[i] = INT_MAX;
		visited[i] = false;
	}
	distance[start] = 0;
	int countOfPoints = 0;
	for (count = 0; count < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 1; count++)
	{
		int min = INT_MAX;
		for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
		if (!visited[i] && distance[i] <= min)
		{
			min = distance[i];
			index = i;
		}
		u = index;
		visited[u] = true;
		for (i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
		if (!visited[i] && (MatrixSmezhnost[u][i] == 1) && distance[u] != INT_MAX &&
			distance[u] + MatrixSmezhnost[u][i] < distance[i])
		{
			distance[i] = distance[u] + MatrixSmezhnost[u][i];
			PointsOfTheWay[SphereNumber][countOfPoints][0] = u;
			PointsOfTheWay[SphereNumber][countOfPoints][1] = i;
			countOfPoints++;
		}
	}
	//Функция задания массива пути
	createWay(SphereNumber, countOfPoints, start, finish);
}

void createMatrix()
{
	int flag1 = 0;
	//Создаем матрицу смежности, как глобальный массив.
	for (int i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
	for (int j = 0; j < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; j++)
		MatrixSmezhnost[i][j] = 0;

	MatrixSmezhnost[0][1] = 1;
	MatrixSmezhnost[1][0] = 1;
	for (int i = 0; i <= GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 1 - 1; i++) //вычли еще одну "1", так как из последней точки не могут выходить ребра.
	{
		while (MatrixSmezhnost[i][i + 1] == 0 && MatrixSmezhnost[i][i + GRAPHPOINTSCOUNTY] == 0 && MatrixSmezhnost[i][i + GRAPHPOINTSCOUNTY + 1] == 0 && i != GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - GRAPHPOINTSCOUNTY - 1) //Проверка
			//чтобы в каждую точку шло ребро
		{
			MatrixSmezhnost[i][i] = 1;
			if (rand() % 6 <= 3)
			if (i % GRAPHPOINTSCOUNTY != GRAPHPOINTSCOUNTY - 1)
			{
				MatrixSmezhnost[i][i + 1] = 1;
				MatrixSmezhnost[i + 1][i] = 1;
			}
			if (i <= GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - GRAPHPOINTSCOUNTY - 1 - 1)
			{
				if (rand() % 6 <= 3)
				if (i <= GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - GRAPHPOINTSCOUNTY - 1)
				{
					MatrixSmezhnost[i][i + GRAPHPOINTSCOUNTY] = 1;
					MatrixSmezhnost[i + GRAPHPOINTSCOUNTY][i] = 1;
				}
				if (rand() % 6 <= 4)
				if (i <= GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - GRAPHPOINTSCOUNTY - 1 - 1 && i % GRAPHPOINTSCOUNTY != GRAPHPOINTSCOUNTY - 1)
				{
					MatrixSmezhnost[i][i + GRAPHPOINTSCOUNTY + 1] = 1;
					MatrixSmezhnost[i + GRAPHPOINTSCOUNTY + 1][i] = 1;
				}
			}
		}
	}
}


void createGraph()
{
	for (int i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
	for (int j = 0; j < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; j++)
	{
		if (MatrixSmezhnost[i][j] == 1 || MatrixSmezhnost[j][i] == 1)
			DrawLine(graphPoints[i / GRAPHPOINTSCOUNTY][i % GRAPHPOINTSCOUNTY], graphPoints[j / GRAPHPOINTSCOUNTY][j % GRAPHPOINTSCOUNTY], NxVec3(1, 1, 1), 0.01F);
	}
}

void createColorMass()
{
	int count = 0;
	for (float i = 0.0; i <= 1.0; i = i + 0.1)
	for (float j = 0.0; j <= 1.0; j = j + 0.1)
	for (float k = 0.0; k <= 1.0; k = k + 0.1)
	{
		colorMass[count] = NxVec3(i, j, k);
		count++;
	}
}

NxVec3 chooseColor(int SphereNumber)
{
	//Не знаю, как адекватно назвать переменные, чтобы было понятно. Реализую алгоритм, который дал Витюков. 
	int del = 1000 / OBJECTSCOUNT;
	int delSphereNumber = del * SphereNumber;
	return colorMass[delSphereNumber];
}

void DrawingLinesOfTheWay(int SphereNumber)
{
	for (int i = 0; WayPoints[SphereNumber][i + 1] != INT_MAX; i++)
	{
		int x1, y1, x2, y2;
		x1 = WayPoints[SphereNumber][i] / GRAPHPOINTSCOUNTY;
		y1 = WayPoints[SphereNumber][i] % GRAPHPOINTSCOUNTY;
		x2 = WayPoints[SphereNumber][i + 1] / GRAPHPOINTSCOUNTY;
		y2 = WayPoints[SphereNumber][i + 1] % GRAPHPOINTSCOUNTY;
		DrawArrow(graphPoints[x1][y1], graphPoints[x2][y2], chooseColor(SphereNumber));
	}
}

int DrawingTheSphere(int SphereNumber, int flag)
{
	if (WayPoints[SphereNumber][(globalCountofI[SphereNumber]) + 1] != INT_MAX)
	{
		int x1, y1, x2, y2;
		x1 = WayPoints[SphereNumber][(globalCountofI[SphereNumber])] / GRAPHPOINTSCOUNTY;
		y1 = WayPoints[SphereNumber][(globalCountofI[SphereNumber])] % GRAPHPOINTSCOUNTY;
		x2 = WayPoints[SphereNumber][(globalCountofI[SphereNumber]) + 1] / GRAPHPOINTSCOUNTY;
		y2 = WayPoints[SphereNumber][(globalCountofI[SphereNumber]) + 1] % GRAPHPOINTSCOUNTY;
		Spheres[SphereNumber]->setGlobalPosition(NxVec3((graphPoints[x1][y1].x + 0.001*OBJECTVELOCITY*globalCount[SphereNumber] * (graphPoints[x2][y2].x - graphPoints[x1][y1].x)),
			2, (graphPoints[x1][y1].z + 0.001*OBJECTVELOCITY*globalCount[SphereNumber] * (graphPoints[x2][y2].z - graphPoints[x1][y1].z)))); //лень менять
		if ((globalCount[SphereNumber] * OBJECTVELOCITY) - 1000 > 0)
		{
			globalCountofI[SphereNumber] = globalCountofI[SphereNumber] + 1;
			globalCount[SphereNumber] = 0;
		}
	}
	else
	{
		globalCount[SphereNumber] = 0;
		globalCountofI[SphereNumber] = 0;
		Pstart[SphereNumber] = Pfinish[SphereNumber];
		while (Pfinish[SphereNumber] == Pstart[SphereNumber] || abs(Pfinish[SphereNumber] - Pstart[SphereNumber]) < (GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY / 3))
			Pfinish[SphereNumber] = rand() % (GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 2) + 1;
		flag = 0;
		for (int i = 0; i < 2000; i++)
		{
			PointsOfTheWay[SphereNumber][i][0] = INT_MAX;
			PointsOfTheWay[SphereNumber][i][1] = INT_MAX;
		}
		for (int i = 0; i < 200; i++)
			WayPoints[SphereNumber][i] = INT_MAX;
	}
	return flag;
}

void RenderCallback()
{
	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ProcessCameraKeys();
	SetupCamera();
	for (int i = 0; i < OBJECTSCOUNT; i++)
		globalCount[i]++;
	if (gScene && !bPause)
	{
		GetPhysicsResults();
		ProcessInputs();
		StartPhysics();
	}

	// Display scene
	RenderActors(bShadows);

	DrawForce(gSelectedActor, gForceVec, NxVec3(0, 0.99, .1));
	gForceVec = NxVec3(0, 0, 0);
	createGraph();
	for (int i = 0; i < OBJECTSCOUNT; i++)
	{
		for (int j = 0; j < 2000; j++)
		{
			PointsOfTheWay[i][j][0] = INT_MAX;
			PointsOfTheWay[i][j][1] = INT_MAX;
		}
		if (flag[i] != 1)
		{
			Dejkstra(i, Pstart[i] - 1, Pfinish[i] - 1);    //вычитаем по 1, т.к. отсчет точек начинается с 0
			flag[i] = 1;
		}
		DrawingLinesOfTheWay(i);
		flag[i] = DrawingTheSphere(i, flag[i]);
	}
	// Render the HUD	
	hud.Render();
	glFlush();
	glutSwapBuffers();
}

void ReshapeCallback(int width, int height)
{
	glViewport(0, 0, width, height);
	gCameraAspectRatio = float(width) / float(height);
}

void IdleCallback()
{
	glutPostRedisplay();
}

void KeyboardCallback(unsigned char key, int x, int y)
{
	gKeys[key] = true;

	switch (key)
	{
	case 'r':	{ SelectNextActor(); break; }
	default:	{ break; }
	}
}

void KeyboardUpCallback(unsigned char key, int x, int y)
{
	gKeys[key] = false;

	switch (key)
	{
	case 'p':
	{
				bPause = !bPause;
				if (bPause)
					hud.SetDisplayString(0, "Paused - Hit \"p\" to Unpause", 0.3f, 0.55f);
				else
					hud.SetDisplayString(0, "", 0.0f, 0.0f);
				getElapsedTime();
				break;
	}
	case 'x': { bShadows = !bShadows; break; }
	case 'b': { bDebugWireframeMode = !bDebugWireframeMode; break; }
	case 27: { exit(0); break; }
	default: { break; }
	}
}

void SpecialCallback(int key, int x, int y)
{
	switch (key)
	{
		// Reset PhysX
	case GLUT_KEY_F10: ResetNx(); return;
	}
}

void MouseCallback(int button, int state, int x, int y)
{
	mx = x;
	my = y;
}

void MotionCallback(int x, int y)
{
	int dx = mx - x;
	int dy = my - y;

	gCameraForward.normalize();
	gCameraRight.cross(gCameraForward, NxVec3(0, 1, 0));

	NxQuat qx(NxPiF32 * dx * 20 / 180.0f, NxVec3(0, 1, 0));
	qx.rotate(gCameraForward);
	NxQuat qy(NxPiF32 * dy * 20 / 180.0f, gCameraRight);
	qy.rotate(gCameraForward);

	mx = x;
	my = y;
}

void ExitCallback()
{
	ReleaseNx();
}

void InitGlut(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitWindowSize(1500, 800);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	gMainHandle = glutCreateWindow("Graph Movement");
	glutSetWindow(gMainHandle);
	glutDisplayFunc(RenderCallback);
	glutReshapeFunc(ReshapeCallback);
	glutIdleFunc(IdleCallback);
	glutKeyboardFunc(KeyboardCallback);
	glutKeyboardUpFunc(KeyboardUpCallback);
	glutSpecialFunc(SpecialCallback);
	glutMouseFunc(MouseCallback);
	glutMotionFunc(MotionCallback);
	MotionCallback(0, 0);
	atexit(ExitCallback);

	// Setup default render states
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_CULL_FACE);

	// Setup lighting
	glEnable(GL_LIGHTING);
	float AmbientColor[] = { 0.0f, 0.1f, 0.2f, 0.0f };         glLightfv(GL_LIGHT0, GL_AMBIENT, AmbientColor);
	float DiffuseColor[] = { 0.2f, 0.2f, 0.2f, 0.0f };         glLightfv(GL_LIGHT0, GL_DIFFUSE, DiffuseColor);
	float SpecularColor[] = { 0.5f, 0.5f, 0.5f, 0.0f };         glLightfv(GL_LIGHT0, GL_SPECULAR, SpecularColor);
	float Position[] = { 100.0f, 100.0f, -400.0f, 1.0f };  glLightfv(GL_LIGHT0, GL_POSITION, Position);
	glEnable(GL_LIGHT0);
}

NxActor* CreateGroundPlane()
{
	// Create a plane with default descriptor
	NxPlaneShapeDesc planeDesc;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&planeDesc);
	return gScene->createActor(actorDesc);
}

NxActor* CreateSphere()
{
	// Set the sphere starting height to 3.5m so box starts off falling onto the ground
	NxReal sphereStartHeight = 3.5;

	// Add a single-shape actor to the scene
	NxActorDesc actorDesc;
	NxBodyDesc bodyDesc;

	// The actor has one shape, a sphere, 1m on radius
	NxSphereShapeDesc sphereDesc;
	sphereDesc.radius = 3;

	actorDesc.shapes.pushBack(&sphereDesc);
	actorDesc.body = &bodyDesc;
	actorDesc.density = 10.0f;
	actorDesc.globalPose.t = NxVec3(3.0f, sphereStartHeight, 0);
	return gScene->createActor(actorDesc);
}

NxActor* CreateBox(NxVec3 ViewPoint)
{
	// Set the box starting height to 3.5m so box starts off falling onto the ground
	NxReal boxStartHeight = 0;

	// Add a single-shape actor to the scene
	NxActorDesc actorDesc;
	NxBodyDesc bodyDesc;

	// The actor has one shape, a box, 1m on a side
	NxBoxShapeDesc boxDesc;
	boxDesc.dimensions.set(2, 2, 2);
	boxDesc.localPose.t = ViewPoint;
	actorDesc.shapes.pushBack(&boxDesc);
	//Тут я чет удалил. Вся функция не нужна. так что пофигу
	actorDesc.body = &bodyDesc;
	actorDesc.density = 10.0f;
	actorDesc.globalPose.t = ViewPoint;
	assert(actorDesc.isValid());
	NxActor *pActor = gScene->createActor(actorDesc);
	assert(pActor);

	return pActor;
}


NxActor* CreateCapsule()
{
	// Set the capsule starting height to 3.5m so box starts off falling onto the ground
	NxReal capsuleStartHeight = 3.5;

	// Add a single-shape actor to the scene
	NxActorDesc actorDesc;
	NxBodyDesc bodyDesc;

	// The actor has one shape, a sphere, 1m on radius
	NxCapsuleShapeDesc capsuleDesc;
	capsuleDesc.radius = 0.55f;
	capsuleDesc.height = 0.75f;
	capsuleDesc.localPose.t = NxVec3(0, 0, 0);

	//Rotate capsule shape
	NxQuat quat(45, NxVec3(0, 0, 1));
	NxMat33 m33(quat);
	capsuleDesc.localPose.M = m33;

	actorDesc.shapes.pushBack(&capsuleDesc);
	actorDesc.body = &bodyDesc;
	actorDesc.density = 10.0f;
	actorDesc.globalPose.t = NxVec3(6.0f, capsuleStartHeight, 0);

	////Rotate actor
	//NxQuat quat1(45, NxVec3(1, 0, 0));
	//NxMat33 m331(quat1);
	//actorDesc.globalPose.M = m331;

	return gScene->createActor(actorDesc);
}

void InitializeHUD()
{
	bHardwareScene = (gScene->getSimType() == NX_SIMULATION_HW);

	hud.Clear();

	//// Add hardware/software to HUD
	//if (bHardwareScene)
	//    hud.AddDisplayString("Hardware Scene", 0.74f, 0.92f);
	//else
	//	hud.AddDisplayString("Software Scene", 0.74f, 0.92f);

	// Add pause to HUD
	if (bPause)
		hud.AddDisplayString("Paused - Hit \"p\" to Unpause", 0.3f, 0.55f);
	else
		hud.AddDisplayString("", 0.0f, 0.0f);
}

void InitNx()
{
	// Initialize camera parameters
	gCameraAspectRatio = 1.0f;
	gCameraPos = NxVec3(STEP * GRAPHPOINTSCOUNTX / 2, 50, -STEP * GRAPHPOINTSCOUNTX);
	gCameraForward = NxVec3(0, 0, 1);
	gCameraRight = NxVec3(-1, 0, 0);

	// Create the physics SDK
	gPhysicsSDK = NxCreatePhysicsSDK(NX_PHYSICS_SDK_VERSION);
	if (!gPhysicsSDK)  return;

	// Set the physics parameters
	gPhysicsSDK->setParameter(NX_SKIN_WIDTH, 0.01);

	// Set the debug visualization parameters
	gPhysicsSDK->setParameter(NX_VISUALIZATION_SCALE, 1);
	gPhysicsSDK->setParameter(NX_VISUALIZE_COLLISION_SHAPES, 1);
	gPhysicsSDK->setParameter(NX_VISUALIZE_ACTOR_AXES, 1);

	// Create the scene
	NxSceneDesc sceneDesc;
	sceneDesc.simType = NX_SIMULATION_SW;
	sceneDesc.gravity = gDefaultGravity;
	gScene = gPhysicsSDK->createScene(sceneDesc);
	if (!gScene)
	{
		sceneDesc.simType = NX_SIMULATION_SW;
		gScene = gPhysicsSDK->createScene(sceneDesc);
		if (!gScene) return;
	}


	// Create the default material
	NxMaterial* defaultMaterial = gScene->getMaterialFromIndex(0);
	defaultMaterial->setRestitution(0.5);
	defaultMaterial->setStaticFriction(0.5);
	defaultMaterial->setDynamicFriction(0.5);
	groundPlane = CreateGroundPlane();
	// Initialize HUD
	InitializeHUD();
	// Get the current time
	getElapsedTime();
	createColorMass();
	for (int i = 0; i < OBJECTSCOUNT; i++)  //создам и задаем начальные параметры каждой сферы
		Spheres[i] = CreateSphere();
	// Start the first frame of the simulation
	if (gScene)  StartPhysics();
}

void ReleaseNx()
{
	if (gScene)
	{
		GetPhysicsResults();  // Make sure to fetchResults() before shutting down
		gPhysicsSDK->releaseScene(*gScene);
	}
	if (gPhysicsSDK)  gPhysicsSDK->release();
}

void ResetNx()
{
	ReleaseNx();
	InitNx();
}

void StartPhysics()
{
	// Update the time step
	gDeltaTime = getElapsedTime();

	// Start collision and dynamics for delta time since the last frame
	gScene->simulate(gDeltaTime);
	gScene->flushStream();
}

void GetPhysicsResults()
{
	// Get results from gScene->simulate(gDeltaTime)
	while (!gScene->fetchResults(NX_RIGID_BODY_FINISHED, false));
}

void RandomingSFPositions()
{
	for (int i = 0; i < OBJECTSCOUNT; i++)  //создам и задаем начальные параметры каждой сферы
	{
		Pstart[i] = Pfinish[i] = 0;
		while (Pfinish[i] == Pstart[i] || Pstart[i] < 1 || Pfinish[i] < 1)
		{
			Pstart[i] = rand() % (GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 1);
			Pfinish[i] = rand() % (GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY - 1);
		}
	}
}

void allocation()
{
	globalCount = new int[OBJECTSCOUNT];
	globalCountofI = new int[OBJECTSCOUNT];
	for (int i = 0; i < OBJECTSCOUNT; i++)
	{
		globalCountofI[i] = 0;
		globalCount[i] = 0;
	}
	Pstart = new int[OBJECTSCOUNT];
	Pfinish = new int[OBJECTSCOUNT];
	flag = new int[OBJECTSCOUNT];

	Spheres = new NxActor*[OBJECTSCOUNT]; //Массив сфер
	graphPoints = new NxVec3*[GRAPHPOINTSCOUNTX];
	for (int i = 0; i < GRAPHPOINTSCOUNTX; i++)
		graphPoints[i] = new NxVec3[GRAPHPOINTSCOUNTY];
	MatrixSmezhnost = new int*[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY];
	for (int i = 0; i < GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY; i++)
		MatrixSmezhnost[i] = new int[GRAPHPOINTSCOUNTX * GRAPHPOINTSCOUNTY];
	PointsOfTheWay = new int**[OBJECTSCOUNT];
	for (int i = 0; i < OBJECTSCOUNT; i++)
	{
		PointsOfTheWay[i] = new int*[2001];
		for (int j = 0; j < 2001; j++)
			PointsOfTheWay[i][j] = new int[2];
	}
	WayPoints = new int*[OBJECTSCOUNT];
	for (int i = 0; i < OBJECTSCOUNT; i++)
		WayPoints[i] = new int[201];
}

bool GetSettings()
{
	TiXmlDocument *xml_file = new TiXmlDocument("AppSettings.xml");
	if (!xml_file->LoadFile())
		return false;

	TiXmlElement* xml_level = 0;
	TiXmlElement* xml_body = 0;
	TiXmlElement* xml_entity = 0;

	xml_level = xml_file->FirstChildElement("level");

	xml_entity = xml_level->FirstChildElement("entity");
	xml_body = xml_entity->FirstChildElement("body");

	printf("1) %s\n", xml_entity->Attribute("class"));
	GRAPHPOINTSCOUNTX = atoi(xml_body->Attribute("GRAPHPOINTSCOUNTX"));
	printf("GraphPointX = %d\n", GRAPHPOINTSCOUNTX);
	GRAPHPOINTSCOUNTY = atoi(xml_body->Attribute("GRAPHPOINTSCOUNTY"));
	printf("GraphPointY = %d\n", GRAPHPOINTSCOUNTY);
	STEP = atoi(xml_body->Attribute("STEP"));
	printf("Step = %d\n", STEP);

	xml_entity = xml_entity->NextSiblingElement("entity");
	xml_body = xml_entity->FirstChildElement("body");

	printf("2) %s\n", xml_entity->Attribute("class"));
	OBJECTSCOUNT = atoi(xml_body->Attribute("OBJECTSCOUNT"));
	printf("ObjectsCount = %d\n", OBJECTSCOUNT);
	OBJECTVELOCITY = atoi(xml_body->Attribute("OBJECTVELOCITY"));
	printf("ObjectsVelocity = %d\n", OBJECTVELOCITY);
	return true;
}

int main(int argc, char** argv)
{
	//setlocale(LC_ALL, "Rus");
	srand(time(NULL));
	GetSettings();
	allocation();
	PrintControls();
	RandomingSFPositions();
	for (int i = 0; i < OBJECTSCOUNT; i++)
	{
		globalCount[OBJECTSCOUNT] = 0;
		flag[i] = 0;
	}
	InitGlut(argc, argv);
	OFFSETX = STEP / 2;
	OFFSETY = STEP / 2;
	createPoints();
	createMatrix();
	InitNx();
	glutMainLoop();
	ReleaseNx();
	return 0;
}
