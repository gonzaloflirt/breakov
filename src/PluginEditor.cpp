/* breakov
 * Copyright (C) 2017 Florian Goltz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "Warnings.h"

PUSH_WARNINGS

namespace breakov
{

Editor::Editor(Processor& p)
  : AudioProcessorEditor(&p)
  , mProcessor(p)
{
  setSize(400, 300);
}

Editor::~Editor()
{
}

void Editor::paint(Graphics& g)
{
  g.fillAll(Colours::darkgrey);
}

void Editor::resized()
{
}

} // namespace breakov

POP_WARNINGS