#include "core/game.h"
#include "render/render.h"
#include "annotation.h"
#include <sstream>

Annotation::Annotation(const std::string& annotation)
{
    std::wstringstream wannotation;
    wannotation << annotation.c_str();
    Game::inst()->render().user_defined_annotation()->BeginEvent(wannotation.str().c_str());
}

Annotation::~Annotation()
{
    Game::inst()->render().user_defined_annotation()->EndEvent();
}
