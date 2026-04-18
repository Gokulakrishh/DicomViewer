#pragma once

#include <memory>

class IMeshData;

class IMeshPostProcessor
{
public:
    virtual ~IMeshPostProcessor() = default;

    [[nodiscard]] virtual std::shared_ptr<IMeshData> process(const IMeshData& mesh) const = 0;
};
