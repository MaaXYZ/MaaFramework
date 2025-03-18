export module maa.nodejs.agent;

import napi;

import maa.nodejs.info;

export void load_agent(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context);
