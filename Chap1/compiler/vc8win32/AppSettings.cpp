#include "tinyxml.h"
#include "tinystr.h"
#include <stdlib.h>
#include <iostream>
#include "AppSettings.h"

using namespace std;

bool GetSettings(int GRAPHPOINTSCOUNTX, int GRAPHPOINTSCOUNTY, int STEP, int OBJECTSCOUNT, int OBJECTVELOCITY)
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