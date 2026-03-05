#pragma once
#include <string>
#include <vector>
#include <memory>

namespace Header
{
    void Render(std::shared_ptr<GameWrapper> gameWrapper, std::vector<std::string>& presetList, int& selectedIndex, bool& needRefresh);
}
