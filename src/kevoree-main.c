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
#include "KMFContainer.h"

#include "lib/list.h"
#include "jsonparse.h"

#include <mcheck.h>

#define DEBUG 1
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define NUM_OF_EXP 27
#define NUM_OF_COMP 100
#define NUM_OF_IT 10

static ContainerRoot *current_model = NULL;
static ContainerRoot *new_model = NULL;
static char modelName[32];

char buffer[100];

void write_to_file(char *buf)
{
	FILE *current_model_json;

	current_model_json = fopen(modelName, "a");

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

/* load model from a json file */
ContainerRoot*
open_model(const char* filename)
{
	struct jsonparse_state jsonState;

	FILE *new_model_json = fopen(filename, "r");
	fseek(new_model_json, 0L, SEEK_END);
	int modelLength = ftell(new_model_json);
	fseek(new_model_json, 0L, SEEK_SET);

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
	jsonparse_setup(&jsonState, jsonModel, modelLength + 1);

	ContainerRoot * model = JSONKevDeserializer(&jsonState, jsonparse_next(&jsonState));
	free(jsonModel);

	return model;
}

/**************************************************************************************************
 * all this is for implementing the checkIfRepository
 **************************************************************************************************/
int
fillLocalAddress(any_t extra, any_t item)
{
	NetworkProperty* s = (NetworkProperty*) item;
	char** address = (char**)extra;
	*address = s->value; 
	return MAP_OK;
}

int
fillAddress(any_t extra, any_t item)
{
	NetworkInfo *nInfo = (NetworkInfo*)item;
	hashmap_iterate(nInfo->values, fillLocalAddress, extra);
	return MAP_OK;
}

struct NodeItem {
	void* next;
	char* nodeName;
	char* nodeIp;
	char* requiredDeployUnit;
	int requiresDeployUnit;
};

int
printComponents(any_t extra, any_t item)
{
	struct NodeItem* nodeItem = (struct NodeItem*) extra;
	ComponentInstance* c = (ComponentInstance*)item;
	TypeDefinition* type = (TypeDefinition*) c->typeDefinition;
	DeployUnit* du = (DeployUnit*)type->deployUnits;
	char* s = typeDefinition_VT.internalGetKey(type);
	char* duName = deployUnit_VT.internalGetKey(du);
	printf("\t%s : %s : %s : du => %s\n", c->name, type->name, s, duName);
	if (strcmp(nodeItem->requiredDeployUnit, duName) == 0) {
		nodeItem->requiresDeployUnit = true;
	}
	return MAP_OK;
}

struct SuperData {
	list_t* l;
	char* requiredDU;
};

int
printNodes(any_t extraData, any_t item)
{
	struct NodeItem* nodeItem = (struct NodeItem*)malloc(sizeof(struct NodeItem));
	ContainerNode *node = (ContainerNode*)item;
	nodeItem->nodeName = node->name;
	hashmap_iterate(node->networkInformation, fillAddress, &nodeItem->nodeIp);

	struct SuperData* data = (struct SuperData*)extraData;

	nodeItem->requiredDeployUnit = data->requiredDU;
	nodeItem->requiresDeployUnit = 0;

	// print components and their type
	printf("Node %s\n", node->name);
	hashmap_iterate(node->components, printComponents, nodeItem);

	if (nodeItem->requiresDeployUnit) {
		// add element to list
		list_t l = *data->l;
		list_add(l, nodeItem);
	}


	return MAP_OK;
}

int compare_ips(char* ip0, char* ip1)
{
	int l = strlen(ip0);
	return (ip0[l-4] == ip1[l-4]) && (ip0[l-3] == ip1[l-3]) && (ip0[l-2] == ip1[l-2]) && (ip0[l-1] == ip1[l-1]);
}

void
filter_nodes(ContainerRoot* model, int children_count, char* childrenIP[], const char* deployUnitType, list_t* r)
{
	struct SuperData superData = { .l = r, .requiredDU=deployUnitType };
	// print address of each node
	hashmap_iterate(model->nodes, printNodes , &superData);

	// filter out elements if they are not in the list of children results
	list_t nodes = *r;
	LIST(tmp);
	list_init(tmp);
	for (int i = 0 ; i < children_count ; i++) {
		struct NodeItem * found = NULL;
		for (struct NodeItem* p =list_head(nodes); p != NULL ; p = list_item_next(p)) {
			if (compare_ips(childrenIP[i], p->nodeIp)) {
				found = p;
			}
		}
		if (found) { 
			list_remove(nodes, found);
			list_push(tmp, found);
		}
	}
	// now just empty the previous list
	while (list_length(nodes) > 0) {
		void* item = list_pop(nodes);
		free(item);
	}

	// now add elements to the empty list
	while (list_length(tmp) > 0) {
		void* item = list_pop(tmp);
		list_push(nodes, item);
	}
}

/********************************************************************************************/
static int
get_map_num(int maxNum)
{
	return (int)((double)rand() / RAND_MAX * maxNum);
}


static void generate_exp_models(char *baseName, int maxComp)
{
	int i;
	int j;
	ContainerRoot *expModel;
	ComponentInstance *comp;
	TypeDefinition *compType;
	hashmap_map *typDefs;
	ContainerNode *node;
	hashmap_map *nodes;
	Dictionary *dico;
	DictionaryValue *dicVal;
	char compName[32];
	int k = 0;
	int mapPos;


	for (i = 1; i <= NUM_OF_EXP; i++) {
		for (j = 0; j < NUM_OF_IT; j++) {
			expModel = open_model("../models/9nodes0component-compactLille.json");
			/*
			 * Add random components to model
			 */

			for (int count = 0 ; count < i ; count++) {
				nodes = (hashmap_map*)expModel->nodes;
				bool found = false;
				while(!found) {
					node = (ContainerNode*)nodes->data[get_map_num(9)].data;
					printf("INFO: Looking for ContainerNode %s\n", node->VT->internalGetKey(node));
					int t = i / 9;
					if ( i % 9 != 0) {
						t++;
					}
					if (hashmap_length(node->components) < t) {
						found = true;
					}
				}

				found = false;
				int randNumb;
				int z;
				hashmap_map *n;
				n = (hashmap_map*)node->components;
				typDefs = (hashmap_map*)expModel->typeDefinitions;
				while (!found) {
					bool tmp = false;
					randNumb = get_map_num(4);
					compType = (TypeDefinition*)typDefs->data[randNumb + 2].data;
					for (z = 0; n != NULL && z < n->table_size; z++) {
						ComponentInstance *ci;
						ci = (ComponentInstance*)n->data[z].data;
						if (ci->typeDefinition == compType) {
							tmp = true;
						}
					}
					found = !tmp;
				}


				printf("INFO: Looking for TypeDefinition %s\n", compType->VT->internalGetKey(compType));

				comp = new_ComponentInstance();
				sprintf(compName, "c%d", k++);
				comp->name = strdup(compName);
				comp->started = true;
				comp->metaData = strdup("");
				comp->VT->addTypeDefinition(comp, compType);

				dico = new_Dictionary();
				dicVal = new_DictionaryValue();
				switch (randNumb) {
				case 2:
					dicVal->name = strdup("name");
					dicVal->value = strdup("Paco");
					break;
				case 3:
					dicVal->name = strdup("count");
					dicVal->value = strdup("10");
					dico->VT->addValues(dico, dicVal);
					dicVal = new_DictionaryValue();
					dicVal->name = strdup("interval");
					dicVal->value = strdup("1000");
					break;
				case 0:
				case 1:
					dicVal->name = strdup("interval");
					dicVal->value = strdup("1000");
					break;

				default:
					break;
				}
				dico->VT->addValues(dico, dicVal);
				comp->VT->addDictionary(comp, dico);

				node->VT->addComponents(node, comp);

			}

			/*
			 * Serialize model to JSON file
			 */
			sprintf(modelName, "%s_%d_%d.json", baseName, i, j);
			expModel->VT->visit(expModel, "", actionstore, NULL, false);
			delete((KMFContainer*)expModel);
			printf("INFO: %s created!\n", modelName);
		}
	}
}

/********************************************************************************************/

static void generate_exp_models2(char *baseName, int maxComp, int numbOfComp)
{
	int i;
	int j;
	ContainerRoot *expModel;
	ComponentInstance *comp;
	TypeDefinition *compType;
	hashmap_map *typDefs;
	ContainerNode *node;
	hashmap_map *nodes;
	Dictionary *dico;
	DictionaryValue *dicVal;
	char compName[32];
	int k = 0;
	int mapPos;

	expModel = open_model("../models/BaseModel.json");


	for (i = 1; i <= numbOfComp; i++) {
		ContainerNode *node = new_ContainerNode();
		node->name = malloc(4);
		sprintf(node->name, "n%d", i);
		node->typeDefinition = expModel->VT->findTypeDefsByID(expModel, "ContikiNode/0.0.1");
		node->metaData = strdup("");
		node->started = true;
		printf("Groupd %s at address %\n", expModel->VT->findGroupsByID(expModel, "group0"));
		if (expModel->VT->findGroupsByID(expModel, "group0") == 0){
			fprintf(stderr, "ERROR: The group was not found\n");	
			exit(1);
		}
		if (node->typeDefinition == 0) {
			fprintf(stderr, "Error locating typedefinition\n");
			exit(1);
		}
		node->VT->addGroups(node, expModel->VT->findGroupsByID(expModel, "group0"));

		NetworkInfo *netInfo = new_NetworkInfo();
		netInfo->name = strdup("ip");

		NetworkProperty *netProp = new_NetworkProperty();
		netProp->name = strdup("local");
		netProp->value = strdup("ip6_number");

		netInfo->VT->addValues(netInfo, netProp);

		node->VT->addNetworkInformation(node, netInfo);

		expModel->VT->addNodes(expModel, node);
	}
		/*
		 * Add random components to model
		 */

		for (int count = 0 ; count < 50 ; count++) {

			printf("Iteration %d\n", count);
			nodes = (hashmap_map*)expModel->nodes;
			bool found = false;
			while(!found) {
				node = (ContainerNode*)nodes->data[get_map_num(50)].data;
				printf("INFO: Looking for ContainerNode %s\n", node->VT->internalGetKey(node));
				int t = 50 / 9;
				if ( 50 % 9 != 0) {
					t++;
				}
				if (hashmap_length(node->components) < t) {
					found = true;
				}
			}

			found = false;
			int randNumb;
			int z;
			hashmap_map *n;
			n = (hashmap_map*)node->components;
			typDefs = (hashmap_map*)expModel->typeDefinitions;
			while (!found) {
				bool tmp = false;
				randNumb = get_map_num(4);
				compType = (TypeDefinition*)typDefs->data[randNumb + 2].data;
				for (z = 0; n != NULL && z < n->table_size; z++) {
					ComponentInstance *ci;
					ci = (ComponentInstance*)n->data[z].data;
					if (ci->typeDefinition == compType) {
						tmp = true;
					}
				}
				found = !tmp;
			}


			printf("INFO: Looking for TypeDefinition %s\n", compType->VT->internalGetKey(compType));

			comp = new_ComponentInstance();
			sprintf(compName, "c%d", k++);
			comp->name = strdup(compName);
			comp->started = true;
			comp->metaData = strdup("");
			comp->VT->addTypeDefinition(comp, compType);

			dico = new_Dictionary();
			dicVal = new_DictionaryValue();
			switch (randNumb) {
			case 2:
				dicVal->name = strdup("name");
				dicVal->value = strdup("Paco");
				break;
			case 3:
				dicVal->name = strdup("count");
				dicVal->value = strdup("10");
				dico->VT->addValues(dico, dicVal);
				dicVal = new_DictionaryValue();
				dicVal->name = strdup("interval");
				dicVal->value = strdup("1000");
				break;
			case 0:
			case 1:
				dicVal->name = strdup("interval");
				dicVal->value = strdup("1000");
				break;

			default:
				break;
			}
			dico->VT->addValues(dico, dicVal);
			comp->VT->addDictionary(comp, dico);

			node->VT->addComponents(node, comp);

		}

		/*
		 * Serialize model to JSON file
		 */
		sprintf(modelName, "%s_%d.json", baseName, 0);
		expModel->VT->visit(expModel, "", actionstore, NULL, false);
		delete((KMFContainer*)expModel);
		printf("INFO: %s created!\n", modelName);
}

/********************************************************************************************/

/********************************************************************************************/
int main(void)
{

	printf("INFO: Starting kevoree C implementation\n");
	/*struct jsonparse_state jsonState;
	TraceSequence *ts;

	jsonparse_setup(&jsonState, DEFAULTMODEL, strlen(DEFAULTMODEL) + 1);
	current_model = JSONKevDeserializer(&jsonState, jsonparse_next(&jsonState));

	//mtrace();

	printf("INFO: Starting Kevoree adaptations\n");
	new_model = open_model("../models/20nodes1component.json");

	//muntrace();

	if(new_model != NULL) {
		printf("INFO: new_model loaded successfully!\n");
	}
	else {
		printf("ERROR: new_model cannot be loaded\n");
	}

	if(new_model != NULL) {
		ts = ModelCompare(new_model, current_model, "n1759");
		if (ts != NULL) {
			/*char *tsString = ts->toString(ts);
			printf("\n%s\n", tsString);
			free(tsString);*/
/*		} else {
			printf("ERROR: Cannot create traceSequence\n");
		}
	}
	else {
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
	delete((KMFContainer*)new_model);

	printf("Info: Loading the Lille's model\n");

	ContainerRoot* model = open_model("../models/9nodes0component-compactLille.json");



	LIST(nodes);
	list_init(nodes);
	char* children[] = {"fe80::3758", "fe80::3554"};
	filter_nodes(model, 2, children, "kev_contiki//blink/0.0.1", &nodes);

	// print the filter results
	for (struct NodeItem* p =list_head(nodes); p != NULL ; p = list_item_next(p)) {
		printf("Info: Node %s has address %s\n", p->nodeName, p->nodeIp);
	}

	//generate_exp_models("../models/generated/lilleM", 0);*/
	generate_exp_models2("../models/cooja/coojaNode", 4, 50);


	return EXIT_SUCCESS;
}
