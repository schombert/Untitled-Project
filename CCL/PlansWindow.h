#pragma once
#include "globalhelpers.h"
#include "SFML\Graphics.hpp"
#include "uielements.hpp"
#include <memory>



void SetupPlansWindow( char_id_t focused) noexcept;
void InitPlansWindow( sf::Font* const font) noexcept;