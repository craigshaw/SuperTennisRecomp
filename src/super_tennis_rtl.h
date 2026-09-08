#pragma once

#include "common_cpu_infra.h"

extern const RtlGameInfo kSuperTennisGameInfo;

int SuperTennisConfigureCapture(const char *directory, unsigned nmi_number);
int SuperTennisLastFrameOk(void);
void SuperTennisDrawPpuFrame(void);
unsigned SuperTennisResumePc(void);
unsigned SuperTennisNmiCount(void);
unsigned SuperTennisLastNmiStack(void);
unsigned SuperTennisLastNmiA(void);
unsigned SuperTennisLastNmiY(void);
unsigned SuperTennisLastNmiP(void);
unsigned long long SuperTennisLastNmiMaster(void);
