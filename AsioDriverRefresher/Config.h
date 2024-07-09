//
// Created by Ahemd Max on 7/8/2024.
//

#pragma once
#include "stdafx.h"


struct AsioConfig
{
    boolean loaded;
    std::array<unsigned, 2> bufferSizes;
};

AsioConfig& GetConfig();

unsigned GetConfigBufferSize(unsigned current);

boolean IsSet(unsigned buffer);