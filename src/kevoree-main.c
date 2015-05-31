#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "kevoree.h"
#include "JSONModelLoader.h"
#include "TraceSequence.h"
#include "ModelCompare.h"
#include "AdaptationPrimitive.h"
#include "Planner.h"
#include "Visitor.h"

#include "lib/list.h"
#include "jsonparse.h"

#include <mcheck.h>

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static ContainerRoot *current_model = NULL;
static ContainerRoot *new_model = NULL;

char buffer[100];

void write_to_file(char *buf)
{
	FILE *current_model_json;

	current_model_json = fopen("current_model.json", "a");

	if(current_model_json)
	{
		fprintf(current_model_json, "%s", buf);
		if(fclose(current_model_json)) {
			printf("ERROR: cannot close the file!\n");
		}
	}
	else
	{
		printf("ERROR: could not write to memory\n");
	}
}

void actionstore(char *path, Type type, void *value)
{
	switch(type)
	{
	case STRING:
		sprintf(buffer, "\"%s\" : \"%s\"", path, (char*)value);
		write_to_file(buffer);
		break;

	case STRREF:
		sprintf(buffer, "\"%s\"", path);
		write_to_file(buffer);
		break;

	case BOOL:
		if ((bool)value == 1) {
			sprintf(buffer, "\"%s\" : \"%s\"", path, "true");
		} else {
			sprintf(buffer, "\"%s\" : \"%s\"", path, "false");
		}
		write_to_file(buffer);
		break;

	case INTEGER:
		sprintf(buffer, "\"%s\" : \"%d\"", path, (int)value);
		write_to_file(buffer);
		break;

	case BRACKET:
		sprintf(buffer, "{\n");
		write_to_file(buffer);
		break;

	case SQBRACKET:
		sprintf(buffer, "\"%s\" : [\n", path);
		write_to_file(buffer);
		break;

	case CLOSEBRACKET:
		sprintf(buffer, "}\n");
		write_to_file(buffer);
		break;

	case CLOSESQBRACKET:
		sprintf(buffer, "]\n");
		write_to_file(buffer);
		break;

	case CLOSEBRACKETCOLON:
		sprintf(buffer, "},\n");
		write_to_file(buffer);
		break;

	case CLOSESQBRACKETCOLON:
		sprintf(buffer, "],\n");
		write_to_file(buffer);
		break;

	case COLON:
		sprintf(buffer, ",\n");
		write_to_file(buffer);
		break;

	case RETURN:
		sprintf(buffer, "\n");
		write_to_file(buffer);
		break;
	}
}

static const char *DEFAULTMODEL = "{\"eClass\":\"org.kevoree.ContainerRoot\",\"generated_KMF_ID\":\"BXX5q3eV\",\"nodes\":[{\"eClass\":\"org.kevoree.ContainerNode\",\"name\":\"n1759\",\"metaData\":\"\",\"started\":\"true\",\"components\":[],\"hosts\":[],\"host\":[],\"groups\":[\"groups[group0]\"],\"networkInformation\":[{\"eClass\":\"org.kevoree.NetworkInfo\",\"name\":\"ip\",\"values\":[{\"eClass\":\"org.kevoree.NetworkProperty\",\"name\":\"local\",\"value\":\"fe80::1759\"}]}],\"typeDefinition\":[\"typeDefinitions[ContikiNode/0.0.1]\"],\"dictionary\":[],\"fragmentDictionary\":[]}],\"typeDefinitions\":[{\"eClass\":\"org.kevoree.NodeType\",\"name\":\"ContikiNode\",\"version\":\"0.0.1\",\"factoryBean\":\"\",\"bean\":\"\",\"abstract\":\"0\",\"deployUnit\":[\"deployUnits[org.kevoree.library.c//kevoree-contiki-node/0.0.1]\"],\"dictionaryType\":[],\"superTypes\":[]},{\"eClass\":\"org.kevoree.GroupType\",\"name\":\"UDPGroup\",\"version\":\"0.0.1\",\"factoryBean\":\"\",\"bean\":\"\",\"abstract\":\"0\",\"deployUnit\":[\"deployUnits[//kevoree-group-udp/0.0.1]\"],\"dictionaryType\":[{\"eClass\":\"org.kevoree.DictionaryType\",\"generated_KMF_ID\":\"3dddTFpd\",\"attributes\":[{\"eClass\":\"org.kevoree.DictionaryAttribute\",\"name\":\"port\",\"optional\":\"false\",\"state\":\"true\",\"datatype\":\"int\",\"fragmentDependant\":\"false\",\"defaultValue\":\"1234\"}]}],\"superTypes\":[]}],\"repositories\":[],\"dataTypes\":[],\"libraries\":[{\"eClass\":\"org.kevoree.TypeLibrary\",\"name\":\"ContikiLib\",\"subTypes\":[\"typeDefinitions[ContikiNode/0.0.1]\",\"typeDefinitions[UDPGroup/0.0.1]\"]},{\"eClass\":\"org.kevoree.TypeLibrary\",\"name\":\"Default\",\"subTypes\":[]}],\"hubs\":[],\"mBindings\":[],\"deployUnits\":[{\"eClass\":\"org.kevoree.DeployUnit\",\"name\":\"kevoree-group-udp\",\"groupName\":\"\",\"version\":\"0.0.1\",\"url\":\"\",\"hashcode\":\"\",\"type\":\"ce\"},{\"eClass\":\"org.kevoree.DeployUnit\",\"name\":\"kevoree-contiki-node\",\"groupName\":\"org.kevoree.library.c\",\"version\":\"0.0.1\",\"url\":\"\",\"hashcode\":\"\",\"type\":\"ce\"}],\"nodeNetworks\":[],\"groups\":[{\"eClass\":\"org.kevoree.Group\",\"name\":\"group0\",\"metaData\":\"\",\"started\":\"true\",\"subNodes\":[\"nodes[n1759]\"],\"typeDefinition\":[\"typeDefinitions[UDPGroup/0.0.1]\"],\"dictionary\":[],\"fragmentDictionary\":[]}]}";

int main(void)
{
	
	printf("INFO: Starting kevoree C implementation\n");
	struct jsonparse_state jsonState;
	TraceSequence *ts;

	jsonparse_setup(&jsonState, DEFAULTMODEL, strlen(DEFAULTMODEL) + 1);
	current_model = JSONKevDeserializer(&jsonState, jsonparse_next(&jsonState));

	/*current_model->VT->visit(current_model, NULL, actionprintf, NULL, false);*/

	printf("INFO: Starting Kevoree adaptations\n");

	FILE *new_model_json = fopen("models/20nodes1component.json", "r");
	fseek(new_model_json, 0L, SEEK_END);
	int modelLength = ftell(new_model_json);
	fseek(new_model_json, 0L, SEEK_SET);

	printf("INFO: Trying to load new_model with length %d\n", modelLength);

	mtrace();

	char *jsonModel = malloc(modelLength + 1);
	int ch;
	bool firstChar = true;
	bool isFirst = true;
	int listLength = 0;

	while ((ch = fgetc(new_model_json)) != EOF) {
		if (firstChar) {
			sprintf(jsonModel, "%c", ch);
			firstChar = false;
		} else {
			sprintf(jsonModel, "%s%c", jsonModel, ch);
		}
	}

	fclose(new_model_json);

	printf("INFO: new_model JSON loaded in RAM\n");

	jsonparse_setup(&jsonState, jsonModel, modelLength + 1);

	new_model = JSONKevDeserializer(&jsonState, jsonparse_next(&jsonState));
	free(jsonModel);
	
	//delete((KMFContainer*)new_model);
	//exit(0);
	
	muntrace();

	/*new_model->VT->visit(new_model, NULL, actionprintf, NULL, false);*/

	if(new_model != NULL)
	{
		printf("INFO: new_model loaded successfully!\n");
	}
	else
	{
		printf("ERROR: new_model cannot be loaded\n");
	}

	/*new_model->VT->visit(new_model, NULL, actionprintf, NULL, false);*/

	if(new_model != NULL)
	{
		ts = ModelCompare(new_model, current_model, "n1759");
		if (ts != NULL) {
			/*char *tsString = ts->toString(ts);
			printf("\n%s\n", tsString);
			free(tsString);*/
		} else {
			printf("ERROR: Cannot create traceSequence\n");
		}
	}
	else
	{
		printf("ERROR: New model cannot be visited!\n");
	}

	LIST(plannedAdaptations);
	list_init(plannedAdaptations);

	Planner_compareModels(current_model, new_model, "n1759", ts);
	plannedAdaptations = Planner_schedule();

	if (plannedAdaptations != NULL) {
		int adaptListLength = list_length(plannedAdaptations);
		PRINTF("INFO: Number of adaptations: %d\n", adaptListLength);
		while (list_length(plannedAdaptations) > 0) {
			AdaptationPrimitive *c = (AdaptationPrimitive*)list_pop(plannedAdaptations);
			char* tmp_path = c->ref->VT->getPath(c->ref);
			printf("%s: Priority: %d Type: %d\n", tmp_path, c->priority, c->primitiveType);
			free(tmp_path);
			c->delete(c);
		}
	} else {
		PRINTF("ERROR: cannot create Adaptation primitives\n");
	}
	ts->delete(ts);

	delete((KMFContainer*)current_model);
	//delete((KMFContainer*)new_model);
	

	return EXIT_SUCCESS;
}
