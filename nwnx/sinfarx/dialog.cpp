#include "dialog.h"

using namespace nwnx::nwscript;
using namespace nwnx::core;
using namespace nwnx::creature;

namespace nwnx { namespace dialog {

enum node_type {StartingNode, EntryNode, ReplyNode};

CNWSDialog* last_dialog = NULL;

unsigned char d_ret_code_rundialogoneliner[0x20];
int (*RunDialogOneLiner_Org)(CNWSObject*, CExoString*, uint32_t) = (int (*)(CNWSObject*, CExoString*, uint32_t))&d_ret_code_rundialogoneliner;
bool running_dialog_one_liner = false;
int RunDialogOneLiner_Hook(CNWSObject* object, CExoString* resref, uint32_t p2)
{
    running_dialog_one_liner = true;
    int ret = RunDialogOneLiner_Org(object, resref, p2);
    running_dialog_one_liner = false;
    return ret;
}
DIALOG_EXTRA* sending_exostring_from_dialog = NULL;
struct SendDialogTextScope
{
    SendDialogTextScope() = delete;
    SendDialogTextScope(CNWSDialog* dialog)
    {
        sending_exostring_from_dialog = (DIALOG_EXTRA*)dialog;
        if (dialog->dlg_private) *(uint16_t*)0x08067EA4 = 0x9090;
        
    }
    ~SendDialogTextScope()
    {
        sending_exostring_from_dialog = NULL;
        *(uint16_t*)0x08067EA4 = 0x1274;
    }
};
unsigned char d_ret_code_senddialogentry[0x20];
int (*SendDialogEntry_Org)(CNWSDialog*, CNWSObject*, uint32_t, uint32_t, int) = (int (*)(CNWSDialog*, CNWSObject*, uint32_t, uint32_t, int))&d_ret_code_senddialogentry;
int SendDialogEntry_HookProc(CNWSDialog* dialog, CNWSObject* object, uint32_t player_id, uint32_t entry_index, int p4)
{
    SendDialogTextScope dlg_text_scope(dialog);
    CGameObject* o_with = GetGameObject(dialog->oid_with);
    CNWSObject* object_with = NULL;
    if (o_with != NULL) object_with = o_with->vtable->AsNWSObject(o_with);
    if (object_with==NULL || 
        o_with->type!=OBJECT_TYPE_CREATURE || 
        dialog->oid_self==dialog->oid_with ||
        get_local_int(object_with, "NO_DIALOG_DELAY"))
    {
        *(uint16_t*)0x0823d4e0 = 0xE990;
    }
    else
    {
        *(uint16_t*)0x0823d4e0 = 0x840f;
    }
    
    if (entry_index < dialog->entries_len)
    {
        if (dialog->entries[entry_index].info.script_conv[0])
        {
            char script_name[17];
            strncpy(script_name, dialog->entries[entry_index].info.script_conv, 16);
            script_name[16] = 0;
            run_script(script_name, object->obj_id);
        }
    }
    
    return SendDialogEntry_Org(dialog, object, player_id, entry_index, p4);
}
int OnSendEntry_CheckActionTakenScript(CResRef* resref, const char* empty_val)
{
    return 1; //== ""
}
unsigned char d_ret_code_senddialogreplies[0x20];
int (*SendDialogReplies_Org)(CNWSDialog*, CNWSObject*, uint32_t) = (int (*)(CNWSDialog*, CNWSObject*, uint32_t))&d_ret_code_senddialogreplies;
int SendDialogReplies_Hook(CNWSDialog* dialog, CNWSObject* object, uint32_t p2)
{
    SendDialogTextScope dlg_text_scope(dialog);
    return SendDialogReplies_Org(dialog, object, p2);
}
int (*HandleReply_Org)(CNWSDialog*, uint32_t, CNWSObject*, uint32_t, int, uint32_t) = (int (*)(CNWSDialog*, uint32_t, CNWSObject*, uint32_t, int, uint32_t))0x823deac;
int HandleReply_HookProc(CNWSDialog* dialog, uint32_t p2, CNWSObject* object, uint32_t p4, int p5, uint32_t p6)
{
    SendDialogTextScope dlg_text_scope(dialog);
    //save last dialog reply timestamp
    if (object && object->obj_type == OBJECT_TYPE_CREATURE)
    {
        GetCreatureExtra((CNWSCreature*)object)->last_dialog_reply_time = time(NULL);
    }
    
    CGameObject* o_with = GetGameObject(dialog->oid_with);
    CNWSObject* object_with = NULL;
    if (o_with != NULL) object_with = o_with->vtable->AsNWSObject(o_with);
    if (object_with==NULL || 
        o_with->type!=OBJECT_TYPE_CREATURE || 
        dialog->oid_self==dialog->oid_with ||
        get_local_int(object_with, "NO_DIALOG_DELAY"))
    {
        *(uint16_t*)0x0823e15d = 0xE990;
    }
    else
    {
        *(uint16_t*)0x0823e15d = 0x840f;
    }
    return HandleReply_Org(dialog, p2, object, p4, p5, p6);
}

void* alloc_cnwsdialog(uint32_t size)
{
    return malloc(sizeof(DIALOG_EXTRA));
}
unsigned char d_ret_code_createdialog[0x20];
int (*CreateDialog_Org)(DIALOG_EXTRA*) = (int (*)(DIALOG_EXTRA*))&d_ret_code_createdialog;
int CreateDialog_Hook(DIALOG_EXTRA* dialog)
{
    if (!running_dialog_one_liner)
    {
        new (dialog) DIALOG_EXTRA();
    }
    return CreateDialog_Org(dialog);
}
unsigned char d_ret_code_cleanupdialog[0x20];
void (*CleanupDialog_Org)(DIALOG_EXTRA*) = (void (*)(DIALOG_EXTRA*))&d_ret_code_cleanupdialog;
void CleanupDialog_Hook(DIALOG_EXTRA* dialog)
{
    if (!running_dialog_one_liner)
    {
        dialog->internal_custom_tokens.clear();
    }
    return CleanupDialog_Org(dialog);
}
unsigned char d_ret_code_destroydialog[0x20];
void (*DestroyDialog_Org)(DIALOG_EXTRA*, int) = (void (*)(DIALOG_EXTRA*, int))&d_ret_code_destroydialog;
void DestroyDialog_Hook(DIALOG_EXTRA* dialog, int p2)
{
    if (dialog == last_dialog) last_dialog = NULL;
    
    DestroyDialog_Org(dialog, p2&~1);
    if (!running_dialog_one_liner)
    {
        if (p2 & 1)
        {
            delete dialog;
        }
        else
        {
            dialog->~DIALOG_EXTRA();			
        }	
    }
}

uint32_t last_selected_node_id = -1;
uint32_t last_selected_absolute_node_id = -1;
unsigned char d_ret_code_conv_node[0x20];
int (*ConversationNodeSelect)(CNWSDialog*, uint32_t, CNWSObject*, uint32_t, int, uint32_t) = (int (*)(CNWSDialog*, uint32_t, CNWSObject*, uint32_t, int, uint32_t))&d_ret_code_conv_node;
int ConversationNodeSelect_HookProc(CNWSDialog* dialog, uint32_t p2, CNWSObject* object, uint32_t selected_node_id, int p5, uint32_t p6)
{
    last_dialog = dialog;
    last_selected_node_id = selected_node_id;
    last_selected_absolute_node_id = -1;

    uint32_t current_node_index = dialog->current_node;
    if(dialog->entries_len > current_node_index)
    {
        CDialogSpeak* current_node = &dialog->entries[current_node_index];
        if(current_node->info.nodes_len > selected_node_id)
        {
            CDialogNode* current_reply = &current_node->info.nodes[selected_node_id];
            last_selected_absolute_node_id = current_reply->index;
        }
    }

    return ConversationNodeSelect(dialog, p2, object, selected_node_id, p5, p6);
}

uint32_t current_node_id = -1;
uint32_t current_node_absolute_id = -1;
int current_node_type = StartingNode;
unsigned char d_ret_code_conv_cond[0x20];
int (*ConditionalScript)(CNWSDialog*, CNWSObject*, CDialogNode*) = (int (*)(CNWSDialog*, CNWSObject*, CDialogNode*))&d_ret_code_conv_cond;
int ConditionalScript_HookProc(CNWSDialog* dialog, CNWSObject* object, CDialogNode* starting_node)
{
    last_dialog = dialog;
    current_node_type = StartingNode;
    current_node_id = starting_node->index;
    current_node_absolute_id = current_node_id;

    bool found = false;
    for(uint32_t entry_index = 0; entry_index < dialog->entries_len; entry_index++)
    {
        CDialogSpeak* entry = &dialog->entries[entry_index];
        for(uint32_t node_index=0; node_index < entry->info.nodes_len; node_index++)
        {
            if(&entry->info.nodes[node_index] == starting_node)
            {
                current_node_type = ReplyNode;
                current_node_id = node_index;
                found = true;
                break;
            }
        }
        if(found == true) break;
    }
    if(found == false)
    {
        for(uint32_t reply_index = 0; reply_index < dialog->replies_len; reply_index++)
        {
            CDialogReply* reply = &dialog->replies[reply_index];
            for(uint32_t node_index=0; node_index < reply->nodes_len; node_index++)
            {
                if(&reply->nodes[node_index] == starting_node)
                {
                    current_node_type = EntryNode;
                    current_node_id = node_index;
                    found = true;
                    break;
                }
            }
            if(found == true) break;
        }
    }
    if(found == false)
    {
        for(uint32_t node_index = 0; node_index < dialog->starts_len; node_index++)
        {
            if(&dialog->starts[node_index] == starting_node)
            {
                current_node_type = StartingNode;
                current_node_id = node_index;
                found = true;
                break;
            }
        }
    }

		return ConditionalScript(dialog, object, starting_node);;
	}
    
void AddAction_OnStartConv(CNWSObject* object, uint32_t p1, uint16_t p2, uint32_t p3, uint32_t* target_id, uint32_t p5, void* p6, uint32_t p7, void* p8, uint32_t p9, void* p10, uint32_t p11, void* p12,
	uint32_t p13, void* p14, uint32_t p15, void* p16, uint32_t p17, void* p18, uint32_t p19, void* p20, uint32_t p21, void* p22, uint32_t p23, void* p24, uint32_t p25, void* p26)
{
	if (object->obj_id == *target_id)
	{
		AddActionToFront(object,p1,p2,p3,target_id,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,
			17,p18,p19,p20,p21,p22,p23,p24,p25,p26);
	}
	else
	{
		AddAction(object,p1,p2,p3,target_id,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15,p16,
			17,p18,p19,p20,p21,p22,p23,p24,p25,p26);
	}
}

std::unordered_map<int, std::string>::iterator internal_custom_tokens_iter;
unsigned char d_ret_code_writeexostring[0x20];
void (*WriteCExoString_Org)(CNWSMessage*, CExoString*, int) = (void (*)(CNWSMessage*, CExoString*, int))&d_ret_code_writeexostring;
void WriteCExoString_HookProc(CNWSMessage* message, CExoString* exo_string, int num_bits)
{

	if (sending_exostring_from_dialog && exo_string->text)
	{
		char* temp = exo_string->text;
		while (*temp)
		{
			if (*temp == '%' && *(temp+1) == '%' && *(temp+2) == '%' && *(temp+3) == '%') break;
			temp++;
		}
		if (*temp)
		{
			char* new_str = (char*)malloc(20000);
			char* new_str_temp = new_str;
			char* copy_start = exo_string->text;
			while (*temp)
			{
				if (*temp == '%' && *(temp+1) == '%' && *(temp+2) == '%' && *(temp+3) == '%')
				{
					int move = temp-copy_start;
					memcpy(new_str_temp, copy_start, move);
					new_str_temp += move;
					temp+=4;
					int token_id = 0;
					while (*temp >= '0' && *temp <= '9')
					{
						token_id *= 10;
						token_id += *temp - '0';
						temp++;
					}
					internal_custom_tokens_iter = sending_exostring_from_dialog->internal_custom_tokens.find(token_id);
					const char* token_replacement;
					if (internal_custom_tokens_iter != sending_exostring_from_dialog->internal_custom_tokens.end())
					{
						token_replacement = internal_custom_tokens_iter->second.c_str();
						move = internal_custom_tokens_iter->second.length();
					}
					else
					{
						token_replacement = "[?]";
						move = 3;
					}
					memcpy(new_str_temp, token_replacement, move);
					new_str_temp += move;
					copy_start = temp;
				}
				else
				{
					temp++;
				}
			}
			memcpy(new_str_temp, copy_start, temp+1-copy_start);
			free(exo_string->text);
			exo_string->text = new_str;
			exo_string->len = 20000;
		}
	}
	return WriteCExoString_Org(message, exo_string, num_bits);
}

void init()
{
    //custom dialog data
    hook_call(0x081D02F5, (long)alloc_cnwsdialog);
    hook_function(0x0823ACE0, (long)CreateDialog_Hook, d_ret_code_createdialog, 10);
    hook_function(0x0823ADA4, (long)CleanupDialog_Hook, d_ret_code_cleanupdialog, 12);
    hook_function(0x081D5790, (long)DestroyDialog_Hook, d_ret_code_destroydialog, 11);

    //do not play an animation if talking with self + run action taken script before sending entry
    hook_function(0x0823d238, (unsigned long)SendDialogEntry_HookProc, d_ret_code_senddialogentry, 12);
    hook_call(0x0823D78B, (long)OnSendEntry_CheckActionTakenScript); //bypass default execute script
    hook_function(0x823D85C, (long)SendDialogReplies_Hook, d_ret_code_senddialogreplies, 12);
    hook_function(0x081D05CC, (long)RunDialogOneLiner_Hook, d_ret_code_rundialogoneliner, 12);
    enable_write(0x08067EA4);	
    //enable write to remove the animation when in a conversation with self
    hook_call(0x081D512F, (uint32_t)HandleReply_HookProc); // need to hook the call because the function is already hooked in nwnx_events
    enable_write(0x0823d4e0); //entry
    enable_write(0x0823e15d);	//reply

    hook_function (0x823deac, (unsigned long)ConversationNodeSelect_HookProc, d_ret_code_conv_node, 12);
    hook_function (0x823cab0, (unsigned long)ConditionalScript_HookProc, d_ret_code_conv_cond, 8);
    
    //start: add action to front when target = self;
    hook_call(0x0820d1f8, (long)AddAction_OnStartConv);
	
	//replace internal tokens
	hook_function(0x80c2d04, (unsigned long)WriteCExoString_HookProc, d_ret_code_writeexostring, 12);
}
REGISTER_INIT(init);

CExoLocStringElement* GetLangEntry(CExoLocString* loc_string, int lang)
{
    if(loc_string->strref != 0xFFFFFFFF) return NULL;
    if (loc_string->list == NULL || loc_string->list->header == NULL) return NULL;
    CExoLinkedListNode* node = loc_string->list->header->first;
    while(node)
    {
        CExoLocStringElement* lang_entry = (CExoLocStringElement*)node->data;
        if(lang_entry->lang == lang) return lang_entry;
        node = node->next;
    }
    return NULL;
}
char* GetStringText(CExoLocString* loc_string, int lang)
{
    if(loc_string->strref != 0xFFFFFFFF) return NULL;
    if (loc_string->list == NULL || loc_string->list->header == NULL) return NULL;
    CExoLinkedListNode* node = loc_string->list->header->first;
    while(node)
    {
        CExoLocStringElement* lang_entry = (CExoLocStringElement*)node->data;
        if(lang_entry->lang == lang) return lang_entry->text.text;
        node = node->next;
    }
    return NULL;
}
VM_FUNC_NEW(GetCurrentNodeType, 418)
{
    vm_push_int(current_node_type);
}
VM_FUNC_NEW(GetCurrentNodeID, 419)
{
    vm_push_int(current_node_id);
}
VM_FUNC_NEW(GetCurrentAbsoluteNodeID, 420)
{
    vm_push_int(current_node_absolute_id);
}
VM_FUNC_NEW(GetSelectedNodeID, 421)
{
    vm_push_int(last_selected_node_id);
}
VM_FUNC_NEW(GetSelectedAbsoluteNodeID, 422)
{
    vm_push_int(last_selected_absolute_node_id);
}
VM_FUNC_NEW(GetSelectedNodeText, 423)
{
    int lang_id = vm_pop_int();
    int gender = vm_pop_int();
    CExoString result;
    if (last_dialog && last_selected_absolute_node_id < last_dialog->replies_len)
    {
        int locale_id = lang_id*2 + gender;
        CExoLocStringElement* loc_string_element = GetLangEntry(&last_dialog->replies[last_selected_absolute_node_id].text, locale_id);
        if (loc_string_element)
        {
            result = *((CExoString*)&loc_string_element->text);
        }
    }
    vm_push_string(&result);
}
VM_FUNC_NEW(GetCurrentNodeText, 424)
{
    int lang_id = vm_pop_int();
    int gender = vm_pop_int();
    CExoString result;
    if (last_dialog)
    {
        int locale_id = lang_id*2 + gender;
        CExoLocString* current_node_text=NULL;
        if((current_node_type == StartingNode || current_node_type == EntryNode) && current_node_absolute_id < last_dialog->entries_len)
        {
            current_node_text = &last_dialog->entries[current_node_absolute_id].info.text;
        }
        else if(current_node_type == ReplyNode && current_node_absolute_id < last_dialog->replies_len)
        {
            current_node_text = &last_dialog->replies[current_node_absolute_id].text;
        }
        CExoLocStringElement* loc_string_element = GetLangEntry(current_node_text, locale_id);
        if (loc_string_element)
        {
            result = *((CExoString*)&loc_string_element->text);
        }
    }
    vm_push_string(&result);
}
VM_FUNC_NEW(SetCurrentNodeText, 425)
{
    CExoString new_text = vm_pop_string();
    int lang_id = vm_pop_int();
    int gender = vm_pop_int();
    if(!last_dialog) return;
    int locale_id = lang_id*2 + gender;
    CExoLocString* current_node_text=NULL;
    if((current_node_type == StartingNode || current_node_type == EntryNode) && current_node_absolute_id < last_dialog->entries_len)
    {
        current_node_text = &last_dialog->entries[current_node_absolute_id].info.text;
    }
    else if(current_node_type == ReplyNode && current_node_absolute_id < last_dialog->replies_len)
    {
        current_node_text = &last_dialog->replies[current_node_absolute_id].text;
    }
    CExoLocStringElement* loc_string_element = GetLangEntry(current_node_text, locale_id);
    if (loc_string_element)
    {
        *((CExoString*)&loc_string_element->text) = new_text;
    }
}
std::unordered_map<int, std::string>* get_tokens_map_from_object_id(uint32_t obj_id)
{
    if (sending_exostring_from_dialog)
    {
        return &(sending_exostring_from_dialog->internal_custom_tokens);
    }
    CGameObject* o = GetGameObject(obj_id);
    if (o && !running_dialog_one_liner)
    {
        CNWSObject* object = o->vtable->AsNWSObject(o);
        if (object)
        {
            if (object->obj_dialog)
            {
                return &(((DIALOG_EXTRA*)object->obj_dialog)->internal_custom_tokens);
            }
            else
            {
                o = GetGameObject(object->obj_conv_owner);
                if (o)
                {
                    CNWSObject* object = o->vtable->AsNWSObject(o);
                    if (object && object->obj_dialog)
                    {
                        return &(((DIALOG_EXTRA*)object->obj_dialog)->internal_custom_tokens);
                    }
                }
            }
        }
    }
    fprintf(stderr, "can't get player for server custom token: %s\n", get_last_script_ran().c_str());
    return NULL;
}
VM_FUNC_NEW(SetDialogCustomToken, 276)
{
    int token_id = vm_pop_int();
    std::string token_value = vm_pop_string();
    std::unordered_map<int, std::string>* tokens_map = get_tokens_map_from_object_id(vm_pop_object());
    if (tokens_map)
    {
        (*tokens_map)[token_id] = token_value;
    }
}
VM_FUNC_NEW(GetDialogCustomToken, 277)
{
    int token_id = vm_pop_int();
    CExoString result;
    std::unordered_map<int, std::string>* tokens_map = get_tokens_map_from_object_id(vm_pop_object());
    if (tokens_map)
    {
        result = CExoString((*tokens_map)[token_id]);
    }
    vm_push_string(&result);
}
	
}
}