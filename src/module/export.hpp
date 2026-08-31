#pragma once

// Экспорт точек входа MTA. Сервер резолвит эти имена через
// LoadLibrary/dlsym при загрузке модуля.

#ifndef MTAEXPORT
#if defined(_WIN32)
#define MTAEXPORT extern "C" __declspec(dllexport)
#else
#define MTAEXPORT extern "C" __attribute__((visibility("default")))
#endif
#endif
