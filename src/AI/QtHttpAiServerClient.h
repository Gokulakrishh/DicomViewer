#pragma once

#include "AI/IAiServerClient.h"

class QtHttpAiServerClient final : public IAiServerClient
{
public:
    AiServerResponse getJson(const AiServerRequest& request) const override;
    AiServerResponse postJson(const AiServerRequest& request) const override;
};
