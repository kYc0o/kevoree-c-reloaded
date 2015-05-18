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
		sprintf(buffer, "\"%s\" : \"%d\"", path, (bool)value);
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

int main(void)
{
	printf("INFO: Starting kevoree C implementation\n");

	current_model = new_ContainerRoot();
	TraceSequence *ts;

	/* ContainerNode contikiNode */
	ContainerNode* contikiNode = new_ContainerNode();
	contikiNode->name = strdup("node0");
	contikiNode->started = true;
	contikiNode->metaData = strdup("");
	
	printf("========>>> %p ========>>>\n", contikiNode->name);

	current_model->VT->addNodes(current_model, contikiNode);

	/* NetworkInfo ip */
	NetworkInfo* serverNodeIP = new_NetworkInfo();
	serverNodeIP->name = strdup("ip");

	contikiNode->VT->addNetworkInformation(contikiNode, serverNodeIP);

	/* NeworkProperty front */
	NetworkProperty* serverNodeFront = new_NetworkProperty();
	serverNodeFront->name = strdup("front");
	serverNodeFront->value = strdup("m3-XX.lille.iotlab.info");

	/* NeworkProperty local */
	NetworkProperty* serverNodeLocal = new_NetworkProperty();
	serverNodeLocal->name = strdup("local");
	serverNodeLocal->value = strdup("aaaa::0:0:5");

	serverNodeIP->VT->addValues(serverNodeIP, serverNodeFront);
	serverNodeIP->VT->addValues(serverNodeIP, serverNodeLocal);

	/*serverNode->VT->addNetworkInformation(serverNode, serverNodeIP);*/

	/* TypeDefinition ContikiNode/1.0.0 */
	TypeDefinition* contikiNodeType = new_NodeType();
	contikiNodeType->name = strdup("ContikiNode");
	contikiNodeType->version = strdup("0.0.1");
	contikiNodeType->bean = strdup("");
	contikiNodeType->factoryBean = strdup("");
	contikiNodeType->abstract = false;

	/*serverNode->VT->addTypeDefinition(serverNode, contikiNodeType);*/

	/* TypeDefinition CoAPGroup/1.0.0 */
	TypeDefinition* coapGroupType = new_GroupType();
	coapGroupType->abstract = false;
	coapGroupType->name = malloc(sizeof(char) * (strlen("CoAPGroup")) + 1);
	strcpy(coapGroupType->name, "CoAPGroup");
	coapGroupType->version = malloc(sizeof(char) * (strlen("0.0.1")) + 1);
	strcpy(coapGroupType->version, "0.0.1");
	coapGroupType->bean = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(coapGroupType->bean, "");
	coapGroupType->factoryBean = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(coapGroupType->factoryBean, "");

	current_model->VT->addTypeDefinitions(current_model, coapGroupType);

	/* TypeLibrary Contiki */
	TypeLibrary* contiki = new_TypeLibrary();
	contiki->name = malloc(sizeof(char) * (strlen("ContikiLib")) + 1);
	strcpy(contiki->name, "ContikiLib");
	/*contiki->VT->addSubTypes(contiki, ctFakeConsole);*/
	/*contiki->VT->addSubTypes(contiki, ctHelloWorld);*/
	contiki->VT->addSubTypes(contiki, contikiNodeType);
	contiki->VT->addSubTypes(contiki, coapGroupType);

	/* TypeLibrary Default */
	TypeLibrary* defLib = new_TypeLibrary();
	defLib->name = malloc(sizeof(char) * (strlen("Default")) + 1);
	strcpy(defLib->name, "Default");

	/* GroupType DictionaryAttribute port */
	DictionaryAttribute* gtDicAttrPort = new_DictionaryAttribute();
	gtDicAttrPort->fragmentDependant = true;
	gtDicAttrPort->optional = true;
	gtDicAttrPort->name = malloc(sizeof(char) * (strlen("port")) + 1);
	strcpy(gtDicAttrPort->name, "port");
	gtDicAttrPort->state = false;
	gtDicAttrPort->datatype = malloc(sizeof(char) * (strlen("number")) + 1);
	strcpy(gtDicAttrPort->datatype, "number");
	gtDicAttrPort->defaultValue = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(gtDicAttrPort->defaultValue, "");

	/* GroupType DictionaryAttribute path */
	DictionaryAttribute* gtDicAttrPath = new_DictionaryAttribute();
	gtDicAttrPath->fragmentDependant = true;
	gtDicAttrPath->optional = true;
	gtDicAttrPath->name = malloc(sizeof(char) * (strlen("path")) + 1);
	strcpy(gtDicAttrPath->name, "path");
	gtDicAttrPath->state = false;
	gtDicAttrPath->datatype = malloc(sizeof(char) * (strlen("string")) + 1);
	strcpy(gtDicAttrPath->datatype, "string");
	gtDicAttrPath->defaultValue = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(gtDicAttrPath->defaultValue, "");

	/* GroupType DictionaryAttribute proxy_port */
	DictionaryAttribute* gtDicAttrProxy = new_DictionaryAttribute();
	gtDicAttrProxy->fragmentDependant = true;
	gtDicAttrProxy->optional = true;
	gtDicAttrProxy->name = malloc(sizeof(char) * (strlen("proxy_port")) + 1);
	strcpy(gtDicAttrProxy->name, "proxy_port");
	gtDicAttrProxy->state = false;
	gtDicAttrProxy->datatype = malloc(sizeof(char) * (strlen("int")) + 1);
	strcpy(gtDicAttrProxy->datatype, "int");
	gtDicAttrProxy->defaultValue = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(gtDicAttrProxy->defaultValue, "20000");

	/* GroupType DictionaryType */
	DictionaryType* gtDicType = new_DictionaryType();
	coapGroupType->VT->addDictionaryType(coapGroupType, gtDicType);

	gtDicType->VT->addAttributes(gtDicType, gtDicAttrPort);
	gtDicType->VT->addAttributes(gtDicType, gtDicAttrPath);
	gtDicType->VT->addAttributes(gtDicType, gtDicAttrProxy);

	/* DeployUnit //kevoree-contiki-node/0.0.1 */
	DeployUnit* kevContikiNode = new_DeployUnit();
	kevContikiNode->name = malloc(sizeof(char) * (strlen("kevoree-contiki-node")) + 1);
	strcpy(kevContikiNode->name, "kevoree-contiki-node");
	kevContikiNode->groupName = malloc(sizeof(char) * (strlen("org.kevoree.library.c")) + 1);
	strcpy(kevContikiNode->groupName, "org.kevoree.library.c");
	kevContikiNode->hashcode = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(kevContikiNode->hashcode, "");
	kevContikiNode->version = malloc(sizeof(char) * (strlen("0.0.1")) + 1);
	strcpy(kevContikiNode->version,"0.0.1");
	kevContikiNode->url = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(kevContikiNode->url, "");
	kevContikiNode->type = malloc(sizeof(char) * (strlen("ce")) + 1);
	strcpy(kevContikiNode->type,"ce");

	/* DeployUnit //kevoree-group-coap/0.0.1 */
	DeployUnit* kevGroupCoap = new_DeployUnit();
	kevGroupCoap->name = malloc(sizeof(char) * (strlen("kevoree-group-coap")) + 1);
	strcpy(kevGroupCoap->name, "kevoree-group-coap");
	kevGroupCoap->groupName = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(kevGroupCoap->groupName, "");
	kevGroupCoap->hashcode = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(kevGroupCoap->hashcode, "");
	kevGroupCoap->version = malloc(sizeof(char) * (strlen("0.0.1")) + 1);
	strcpy(kevGroupCoap->version,"0.0.1");
	kevGroupCoap->url = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(kevGroupCoap->url, "");
	kevGroupCoap->type = malloc(sizeof(char) * (strlen("ce")) + 1);
	strcpy(kevGroupCoap->type,"ce");

	/* Group CoAP */
	Group* coapGroup = new_Group();
	/*coapGroup->name = malloc(sizeof(char) * (strlen("group0")) + 1);
	strcpy(coapGroup->name, "group0");*/
	coapGroup->name = strdup("group0");
	coapGroup->started = true;
	coapGroup->metaData = malloc(sizeof(char) * (strlen("")) + 1);
	strcpy(coapGroup->metaData, "");

	current_model->VT->addGroups(current_model, coapGroup);
	coapGroup->VT->addTypeDefinition(coapGroup, coapGroupType);

	/* FragmentDictionary contiki-node */
	FragmentDictionary* coapGroupFragDico = new_FragmentDictionary();
	coapGroupFragDico->name = malloc(sizeof(char) * (strlen("CoAPGroupFragDic")) + 1);
	strcpy(coapGroupFragDico->name, "CoAPGroupFragDic");

	/* Group DictionaryValue port */
	DictionaryValue* groupValuePort = new_DictionaryValue();
	groupValuePort->name = malloc(sizeof(char) * (strlen("port")) + 1);
	strcpy(groupValuePort->name, "port");
	groupValuePort->value = malloc(sizeof(char) * (strlen("5683")) + 1);
	strcpy(groupValuePort->value, "5683");

	/* Group DictionaryValue proxy_port */
	DictionaryValue* groupValueProxy = new_DictionaryValue();
	groupValueProxy->name = malloc(sizeof(char) * (strlen("proxy_port")) + 1);
	strcpy(groupValueProxy->name, "proxy_port");
	groupValueProxy->value = malloc(sizeof(char) * (strlen("20000")) + 1);
	strcpy(groupValueProxy->value, "20000");

	/* Group DictionaryValue path */
	DictionaryValue* groupValuePath = new_DictionaryValue();
	groupValuePath->name = malloc(sizeof(char) * (strlen("path")) + 1);
	strcpy(groupValuePath->name, "path");
	groupValuePath->value = malloc(sizeof(char) * (strlen("CoAPGroup")) + 1);
	strcpy(groupValuePath->value, "CoAPGroup");

	coapGroup->VT->addFragmentDictionary(coapGroup, coapGroupFragDico);
	/* Adding values to FragmentDictionary ContikiNode */
	coapGroupFragDico->VT->addValues(coapGroupFragDico, groupValuePort);
	coapGroupFragDico->VT->addValues(coapGroupFragDico, groupValueProxy);
	coapGroupFragDico->VT->addValues(coapGroupFragDico, groupValuePath);

	contikiNodeType->VT->addDeployUnit(contikiNodeType, kevContikiNode);
	coapGroupType->VT->addDeployUnit(coapGroupType, kevGroupCoap);

	current_model->VT->addLibraries(current_model, contiki);
	current_model->VT->addLibraries(current_model, defLib);

	/*type definition*/
	current_model->VT->addTypeDefinitions(current_model, contikiNodeType);

	/*deploy unit*/
	current_model->VT->addDeployUnits(current_model, kevContikiNode);
	current_model->VT->addDeployUnits(current_model, kevGroupCoap);

	/*contiki->VT->addSubTypes(contiki, coapChanType);*/
	contikiNode->VT->addTypeDefinition(contikiNode, contikiNodeType);
	contikiNode->VT->addGroups(contikiNode, coapGroup);
	coapGroup->VT->addSubNodes(coapGroup, contikiNode);

	/*current_model->VT->visit(current_model, NULL, actionprintf, NULL, false);*/

	printf("INFO: Starting Kevoree adaptations\n");
	mtrace();

	FILE *new_model_json = fopen("../models/new_model-compact.json", "r");
	fseek(new_model_json, 0L, SEEK_END);
	int modelLength = ftell(new_model_json);
	fseek(new_model_json, 0L, SEEK_SET);

	printf("INFO: Trying to load new_model with length %d\n", modelLength);

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

	struct jsonparse_state jsonState;

	jsonparse_setup(&jsonState, jsonModel, modelLength + 1);

	new_model = JSONKevDeserializer(&jsonState, jsonparse_next(&jsonState));
	free(jsonModel);

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
		ts = ModelCompare(new_model, current_model);
		if (ts != NULL) {
			printf("\n%s\n", ts->toString(ts));
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

	Planner_compareModels(current_model, new_model, "node0", ts);
	plannedAdaptations = Planner_schedule();

	if (plannedAdaptations != NULL) {
		int adaptListLength = list_length(plannedAdaptations);
		PRINTF("INFO: Number of adaptations: %d\n", adaptListLength);
		while (list_length(plannedAdaptations) > 0) {
			AdaptationPrimitive *c = (AdaptationPrimitive*)list_pop(plannedAdaptations);
			printf("%s: Priority: %d Type: %d\n", c->ref->path, c->priority, c->primitiveType);
		}
	} else {
		PRINTF("ERROR: cannot create Adaptation primitives\n");
	}

	delete((KMFContainer*)current_model);
	delete((KMFContainer*)new_model);
	muntrace();

	return EXIT_SUCCESS;
}
