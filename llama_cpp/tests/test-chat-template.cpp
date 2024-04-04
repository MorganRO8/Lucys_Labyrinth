#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#undef NDEBUG
#include <cassert>

#include "llama.h"

int main(void) {
    llama_chat_message conversation[] = {
        {"system", "You are a helpful assistant"},
        {"user", "Hello"},
        {"assistant", "Hi there"},
        {"user", "Who are you"},
        {"assistant", "   I am an assistant   "},
        {"user", "Another question"},
    };
    size_t message_count = 6;
    std::vector<std::string> templates = {
        // teknium/OpenHermes-2.5-Mistral-7B
        "{% for message in messages %}{{'<|im_start|>' + message['role'] + '\\n' + message['content'] + '<|im_end|>' + '\\n'}}{% endfor %}{% if add_generation_prompt %}{{ '<|im_start|>assistant\\n' }}{% endif %}",
        // mistralai/Mistral-7B-Instruct-v0.2
        "{{ bos_token }}{% for message in messages %}{% if (message['role'] == 'user') != (loop.index0 % 2 == 0) %}{{ raise_exception('Conversation roles must alternate user/assistant/user/assistant/...') }}{% endif %}{% if message['role'] == 'user' %}{{ '[INST] ' + message['content'] + ' [/INST]' }}{% elif message['role'] == 'assistant' %}{{ message['content'] + eos_token}}{% else %}{{ raise_exception('Only user and assistant roles are supported!') }}{% endif %}{% endfor %}",
        // TheBloke/FusionNet_34Bx2_MoE-AWQ
        "{%- for idx in range(0, messages|length) -%}\\n{%- if messages[idx]['role'] == 'user' -%}\\n{%- if idx > 1 -%}\\n{{- bos_token + '[INST] ' + messages[idx]['content'] + ' [/INST]' -}}\\n{%- else -%}\\n{{- messages[idx]['content'] + ' [/INST]' -}}\\n{%- endif -%}\\n{% elif messages[idx]['role'] == 'system' %}\\n{{- '[INST] <<SYS>>\\\\n' + messages[idx]['content'] + '\\\\n<</SYS>>\\\\n\\\\n' -}}\\n{%- elif messages[idx]['role'] == 'assistant' -%}\\n{{- ' '  + messages[idx]['content'] + ' ' + eos_token -}}\\n{% endif %}\\n{% endfor %}",
        // bofenghuang/vigogne-2-70b-chat
        "{{ bos_token }}{% if messages[0]['role'] == 'system' %}{% set loop_messages = messages[1:] %}{% set system_message = messages[0]['content'] %}{% elif true == true and not '<<SYS>>' in messages[0]['content'] %}{% set loop_messages = messages %}{% set system_message = 'Vous êtes Vigogne, un assistant IA créé par Zaion Lab. Vous suivez extrêmement bien les instructions. Aidez autant que vous le pouvez.' %}{% else %}{% set loop_messages = messages %}{% set system_message = false %}{% endif %}{% for message in loop_messages %}{% if (message['role'] == 'user') != (loop.index0 % 2 == 0) %}{{ raise_exception('Conversation roles must alternate user/assistant/user/assistant/...') }}{% endif %}{% if loop.index0 == 0 and system_message != false %}{% set content = '<<SYS>>\\\\n' + system_message + '\\\\n<</SYS>>\\\\n\\\\n' + message['content'] %}{% else %}{% set content = message['content'] %}{% endif %}{% if message['role'] == 'user' %}{{ '[INST] ' + content.strip() + ' [/INST]' }}{% elif message['role'] == 'system' %}{{ '<<SYS>>\\\\n' + content.strip() + '\\\\n<</SYS>>\\\\n\\\\n' }}{% elif message['role'] == 'assistant' %}{{ ' '  + content.strip() + ' ' + eos_token }}{% endif %}{% endfor %}",
        // mlabonne/AlphaMonarch-7B
        "{% for message in messages %}{{bos_token + message['role'] + '\\n' + message['content'] + eos_token + '\\n'}}{% endfor %}{% if add_generation_prompt %}{{ bos_token + 'assistant\\n' }}{% endif %}",
        // google/gemma-7b-it
        "{% if messages[0]['role'] == 'system' %}{{ raise_exception('System role not supported') }}{% endif %}{% for message in messages %}{% if (message['role'] == 'user') != (loop.index0 % 2 == 0) %}{{ raise_exception('Conversation roles must alternate user/assistant/user/assistant/...') }}{% endif %}{% if (message['role'] == 'assistant') %}{% set role = 'model' %}{% else %}{% set role = message['role'] %}{% endif %}{{ '<start_of_turn>' + role + '\\n' + message['content'] | trim + '<end_of_turn>\\n' }}{% endfor %}{% if add_generation_prompt %}{{'<start_of_turn>model\\n'}}{% endif %}",
    };
    std::vector<std::string> expected_output = {
        // teknium/OpenHermes-2.5-Mistral-7B
        "<|im_start|>system\nYou are a helpful assistant<|im_end|>\n<|im_start|>user\nHello<|im_end|>\n<|im_start|>assistant\nHi there<|im_end|>\n<|im_start|>user\nWho are you<|im_end|>\n<|im_start|>assistant\n   I am an assistant   <|im_end|>\n<|im_start|>user\nAnother question<|im_end|>\n<|im_start|>assistant\n",
        // mistralai/Mistral-7B-Instruct-v0.2
        "[INST] You are a helpful assistant\nHello [/INST]Hi there</s>[INST] Who are you [/INST]   I am an assistant   </s>[INST] Another question [/INST]",
        // TheBloke/FusionNet_34Bx2_MoE-AWQ
        "[INST] <<SYS>>\nYou are a helpful assistant\n<</SYS>>\n\nHello [/INST] Hi there </s><s>[INST] Who are you [/INST]    I am an assistant    </s><s>[INST] Another question [/INST]",
        // bofenghuang/vigogne-2-70b-chat
        "[INST] <<SYS>>\nYou are a helpful assistant\n<</SYS>>\n\nHello [/INST] Hi there </s>[INST] Who are you [/INST] I am an assistant </s>[INST] Another question [/INST]",
        // mlabonne/AlphaMonarch-7B
        "system\nYou are a helpful assistant</s>\n<s>user\nHello</s>\n<s>assistant\nHi there</s>\n<s>user\nWho are you</s>\n<s>assistant\n   I am an assistant   </s>\n<s>user\nAnother question</s>\n<s>assistant\n",
        // google/gemma-7b-it
        "<start_of_turn>user\nYou are a helpful assistant\n\nHello<end_of_turn>\n<start_of_turn>model\nHi there<end_of_turn>\n<start_of_turn>user\nWho are you<end_of_turn>\n<start_of_turn>model\nI am an assistant<end_of_turn>\n<start_of_turn>user\nAnother question<end_of_turn>\n<start_of_turn>model\n",
    };
    std::vector<char> formatted_chat(1024);
    int32_t res;

    // test invalid chat template
    res = llama_chat_apply_template(nullptr, "INVALID TEMPLATE", conversation, message_count, true, formatted_chat.data(), formatted_chat.size());
    assert(res < 0);

    for (size_t i = 0; i < templates.size(); i++) {
        std::string custom_template = templates[i];
        std::string expected = expected_output[i];
        formatted_chat.resize(1024);
        res = llama_chat_apply_template(
            nullptr,
            custom_template.c_str(),
            conversation,
            message_count,
            true,
            formatted_chat.data(),
            formatted_chat.size()
        );
        formatted_chat.resize(res);
        std::string output(formatted_chat.data(), formatted_chat.size());
        std::cout << output << "\n-------------------------\n";
        assert(output == expected);
    }
    return 0;
}
