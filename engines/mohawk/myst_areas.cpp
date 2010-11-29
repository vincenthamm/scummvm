/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "mohawk/graphics.h"
#include "mohawk/myst_areas.h"
#include "mohawk/myst_scripts.h"
#include "mohawk/video.h"

namespace Mohawk {

MystResource::MystResource(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) {
	_vm = vm;
	_parent = parent;

	if (parent == NULL) {
		_flags = rlstStream->readUint16LE();
		_rect.left = rlstStream->readSint16LE();
		_rect.top = rlstStream->readSint16LE();

		if (_rect.top == -1) {
			warning("Invalid _rect.top of -1 found - clamping to 0");
			_rect.top = 0;
		}

		_rect.right = rlstStream->readSint16LE();
		_rect.bottom = rlstStream->readSint16LE();
		_dest = rlstStream->readUint16LE();
	} else {
		_flags = parent->_flags;
		_rect.left = parent->_rect.left;
		_rect.top = parent->_rect.top;
		_rect.right = parent->_rect.right;
		_rect.bottom = parent->_rect.bottom;
		_dest = parent->_dest;
	}

	debugC(kDebugResource, "\tflags: 0x%04X", _flags);
	debugC(kDebugResource, "\tleft: %d", _rect.left);
	debugC(kDebugResource, "\ttop: %d", _rect.top);
	debugC(kDebugResource, "\tright: %d", _rect.right);
	debugC(kDebugResource, "\tbottom: %d", _rect.bottom);
	debugC(kDebugResource, "\tdest: %d", _dest);

	// Default Enable based on flags...
	if (_vm->_zipMode)
		_enabled = (_flags & kMystZipModeEnableFlag) != 0 ||
		           (_flags & kMystHotspotEnableFlag) != 0 ||
		           (_flags & kMystSubimageEnableFlag) != 0;
	else
		_enabled = (_flags & kMystZipModeEnableFlag) == 0 &&
		           ((_flags & kMystHotspotEnableFlag) != 0 ||
		            (_flags & kMystSubimageEnableFlag) != 0);
}

MystResource::~MystResource() {
}

void MystResource::handleMouseUp() {
	if (_dest != 0)
		_vm->changeToCard(_dest);
	else
		warning("Movement type resource with null destination at position (%d, %d), (%d, %d)", _rect.left, _rect.top, _rect.right, _rect.bottom);
}

MystResourceType5::MystResourceType5(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResource(vm, rlstStream, parent) {
	debugC(kDebugResource, "\tResource Type 5 Script:");

	_script = vm->_scriptParser->readScript(rlstStream, kMystScriptNormal);
}

void MystResourceType5::handleMouseUp() {

//	MystResource *invoking = this;
//	while (invoking->_parent) {
//		invoking = invoking->_parent;
//	}

	_vm->_scriptParser->runScript(_script, this);
}

// In Myst/Making of Myst, the paths are hardcoded ala Windows style without extension. Convert them.
Common::String MystResourceType6::convertMystVideoName(Common::String name) {
	Common::String temp;

	for (uint32 i = 1; i < name.size(); i++) {
		if (name[i] == '\\')
			temp += '/';
		else
			temp += name[i];
	}

	return temp + ".mov";
}

MystResourceType6::MystResourceType6(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResourceType5(vm, rlstStream, parent) {
	char c = 0;

	do {
		c = rlstStream->readByte();
		_videoFile += c;
	} while (c);

	rlstStream->skip(_videoFile.size() & 1);

	// Trim method does not remove extra trailing nulls
	while (_videoFile.size() != 0 && _videoFile.lastChar() == 0)
		_videoFile.deleteLastChar();

	_videoFile = convertMystVideoName(_videoFile);

	// Position values require modulus 10000 to keep in sane range.
	_left = rlstStream->readUint16LE() % 10000;
	_top = rlstStream->readUint16LE() % 10000;
	_loop = rlstStream->readUint16LE();
	_u0 = rlstStream->readUint16LE();
	_playBlocking = rlstStream->readUint16LE();
	_playOnCardChange = rlstStream->readUint16LE();
	_u3 = rlstStream->readUint16LE();

	if (_u0 != 1)
		warning("Type 6 _u0 != 1");
	if (_u3 != 0)
		warning("Type 6 _u3 != 0");

	debugC(kDebugResource, "\tvideoFile: \"%s\"", _videoFile.c_str());
	debugC(kDebugResource, "\tleft: %d", _left);
	debugC(kDebugResource, "\ttop: %d", _top);
	debugC(kDebugResource, "\tloop: %d", _loop);
	debugC(kDebugResource, "\tu0: %d", _u0);
	debugC(kDebugResource, "\tplayBlocking: %d", _playBlocking);
	debugC(kDebugResource, "\tplayOnCardChange: %d", _playOnCardChange);
	debugC(kDebugResource, "\tu3: %d", _u3);

	_videoRunning = false;
}

void MystResourceType6::handleAnimation() {
	// TODO: Implement Code to allow _playOnCardChange when set
	//       and trigger by Opcode 9 when clear

	if (!_videoRunning) {
		// NOTE: The left and top coordinates are often incorrect and do not make sense.
		// We use the rect coordinates here instead.

		if (_playBlocking)
			_vm->_video->playMovie(_videoFile, _rect.left, _rect.top);
		else
			_vm->_video->playBackgroundMovie(_videoFile, _rect.left, _rect.top, _loop);

		_videoRunning = true;
	}
}

MystResourceType7::MystResourceType7(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResource(vm, rlstStream, parent) {
	_var7 = rlstStream->readUint16LE();
	_numSubResources = rlstStream->readUint16LE();
	debugC(kDebugResource, "\tvar7: %d", _var7);
	debugC(kDebugResource, "\tnumSubResources: %d", _numSubResources);

	for (uint16 i = 0; i < _numSubResources; i++)
		_subResources.push_back(vm->loadResource(rlstStream, this));
}

MystResourceType7::~MystResourceType7() {
	while(!_subResources.empty()) {
		MystResource *temp = _subResources.back();
		_subResources.pop_back();
		delete temp;
	}
}

// TODO: All these functions to switch subresource are very similar.
//       Find way to share code (function pointer pass?)
void MystResourceType7::drawDataToScreen() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->drawDataToScreen();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->drawDataToScreen();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->drawDataToScreen();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

void MystResourceType7::handleAnimation() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->handleAnimation();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->handleAnimation();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->handleAnimation();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

void MystResourceType7::handleMouseUp() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->handleMouseUp();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->handleMouseUp();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->handleMouseUp();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

void MystResourceType7::handleMouseDown() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->handleMouseDown();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->handleMouseDown();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->handleMouseDown();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

void MystResourceType7::handleMouseEnter() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->handleMouseEnter();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->handleMouseEnter();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->handleMouseEnter();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

void MystResourceType7::handleMouseLeave() {
	if (_var7 == 0xFFFF) {
		if (_numSubResources == 1)
			_subResources[0]->handleMouseLeave();
		else if (_numSubResources != 0)
			warning("Type 7 Resource with _numSubResources of %d, but no control variable", _numSubResources);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var7);

		if (_numSubResources == 1 && varValue != 0)
			_subResources[0]->handleMouseLeave();
		else if (_numSubResources != 0) {
			if (varValue < _numSubResources)
				_subResources[varValue]->handleMouseLeave();
			else
				warning("Type 7 Resource Var %d: %d exceeds number of sub resources %d", _var7, varValue, _numSubResources);
		}
	}
}

MystResourceType8::MystResourceType8(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResourceType7(vm, rlstStream, parent) {
	_var8 = rlstStream->readUint16LE();
	_numSubImages = rlstStream->readUint16LE();
	debugC(kDebugResource, "\tvar8: %d", _var8);
	debugC(kDebugResource, "\tnumSubImages: %d", _numSubImages);

	_subImages = new MystResourceType8::SubImage[_numSubImages];
	for (uint16 i = 0; i < _numSubImages; i++) {
		debugC(kDebugResource, "\tSubimage %d:", i);

		_subImages[i].wdib = rlstStream->readUint16LE();
		_subImages[i].rect.left = rlstStream->readSint16LE();

		if (_subImages[i].rect.left != -1) {
			_subImages[i].rect.top = rlstStream->readSint16LE();
			_subImages[i].rect.right = rlstStream->readSint16LE();
			_subImages[i].rect.bottom = rlstStream->readSint16LE();
		} else {
			// Use the hotspot rect as the source rect since the subimage is fullscreen
			// Convert to bitmap coordinates (upside down)
			_subImages[i].rect.left = _rect.left;
			_subImages[i].rect.top = 333 - _rect.bottom;
			_subImages[i].rect.right = _rect.right;
			_subImages[i].rect.bottom = 333 - _rect.top;
		}

		debugC(kDebugResource, "\twdib: %d", _subImages[i].wdib);
		debugC(kDebugResource, "\tleft: %d", _subImages[i].rect.left);
		debugC(kDebugResource, "\ttop: %d", _subImages[i].rect.top);
		debugC(kDebugResource, "\tright: %d", _subImages[i].rect.right);
		debugC(kDebugResource, "\tbottom: %d", _subImages[i].rect.bottom);
	}
}

MystResourceType8::~MystResourceType8() {
	delete[] _subImages;
}

void MystResourceType8::drawDataToScreen() {
	// Need to call overidden Type 7 function to ensure
	// switch section is processed correctly.
	MystResourceType7::drawDataToScreen();

	bool drawSubImage = false;
	int16 subImageId = 0;

	if (_var8 == 0xFFFF) {
		if (_numSubImages == 1) {
			subImageId = 0;
			drawSubImage = true;
		} else if (_numSubImages != 0)
			warning("Type 8 Resource with _numSubImages of %d, but no control variable", _numSubImages);
	} else {
		uint16 varValue = _vm->_scriptParser->getVar(_var8);

		if (_numSubImages == 1 && varValue != 0) {
			subImageId = 0;
			drawSubImage = true;
		} else if (_numSubImages != 0) {
			if (varValue < _numSubImages) {
				subImageId = varValue;
				drawSubImage = true;
			} else
				warning("Type 8 Image Var %d: %d exceeds number of subImages %d", _var8, varValue, _numSubImages);
		}
	}

	if (drawSubImage) {
		uint16 imageToDraw = 0;

		if (_subImages[subImageId].wdib == 0xFFFF) {
			// TODO: Think the reason for problematic screen updates in some rects is that they
			//       are these -1 cases.
			// They need to be redrawn i.e. if the Myst marker switches are changed, but I don't think
			// the rects are valid. This does not matter in the original engine as the screen update redraws
			// the VIEW images, followed by the RLST resource images, and -1 for the WDIB is interpreted as
			// "Do Not Draw Image" i.e so the VIEW image is shown through.. We need to fix screen update
			// to do this same behaviour.
			if (_vm->_view.conditionalImageCount == 0)
				imageToDraw = _vm->_view.mainImage;
			else {
				for (uint16 i = 0; i < _vm->_view.conditionalImageCount; i++)
					if (_vm->_scriptParser->getVar(_vm->_view.conditionalImages[i].var) < _vm->_view.conditionalImages[i].numStates)
						imageToDraw = _vm->_view.conditionalImages[i].values[_vm->_scriptParser->getVar(_vm->_view.conditionalImages[i].var)];
			}
		} else
			imageToDraw = _subImages[subImageId].wdib;

		_vm->_gfx->copyImageSectionToScreen(imageToDraw, _subImages[subImageId].rect, _rect);
	}
}

void MystResourceType8::drawConditionalDataToScreen(uint16 state) {
	// Need to call overidden Type 7 function to ensure
	// switch section is processed correctly.
	MystResourceType7::drawDataToScreen();

	bool drawSubImage = false;
	int16 subImageId = 0;


	if (_numSubImages == 1 && state != 0) {
		subImageId = 0;
		drawSubImage = true;
	} else if (_numSubImages != 0) {
		if (state < _numSubImages) {
			subImageId = state;
			drawSubImage = true;
		} else
			warning("Type 8 Image Var %d: %d exceeds number of subImages %d", _var8, state, _numSubImages);
	}


	if (drawSubImage) {
		uint16 imageToDraw = 0;

		if (_subImages[subImageId].wdib == 0xFFFF) {
			// TODO: Think the reason for problematic screen updates in some rects is that they
			//       are these -1 cases.
			// They need to be redrawn i.e. if the Myst marker switches are changed, but I don't think
			// the rects are valid. This does not matter in the original engine as the screen update redraws
			// the VIEW images, followed by the RLST resource images, and -1 for the WDIB is interpreted as
			// "Do Not Draw Image" i.e so the VIEW image is shown through.. We need to fix screen update
			// to do this same behaviour.
			if (_vm->_view.conditionalImageCount == 0)
				imageToDraw = _vm->_view.mainImage;
			else {
				for (uint16 i = 0; i < _vm->_view.conditionalImageCount; i++)
					if (_vm->_scriptParser->getVar(_vm->_view.conditionalImages[i].var) < _vm->_view.conditionalImages[i].numStates)
						imageToDraw = _vm->_view.conditionalImages[i].values[_vm->_scriptParser->getVar(_vm->_view.conditionalImages[i].var)];
			}
		} else
			imageToDraw = _subImages[subImageId].wdib;

		_vm->_gfx->copyImageSectionToScreen(imageToDraw, _subImages[subImageId].rect, _rect);
	}
}

uint16 MystResourceType8::getType8Var() {
	return _var8;
}

// No MystResourceType9!

MystResourceType10::MystResourceType10(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResourceType8(vm, rlstStream, parent) {
	_kind = rlstStream->readUint16LE();
	// NOTE: l,r,t,b differs from standard l,t,r,b order
	_rect10.left = rlstStream->readUint16LE();
	_rect10.right = rlstStream->readUint16LE();
	_rect10.top = rlstStream->readUint16LE();
	_rect10.bottom = rlstStream->readUint16LE();
	_u0 = rlstStream->readUint16LE();
	_u1 = rlstStream->readUint16LE();
	_mouseDownOpcode = rlstStream->readUint16LE();
	_mouseDragOpcode = rlstStream->readUint16LE();
	_mouseUpOpcode = rlstStream->readUint16LE();

	// TODO: Need to work out meaning of kind...
	debugC(kDebugResource, "\tkind: %d", _kind);
	debugC(kDebugResource, "\trect10.left: %d", _rect10.left);
	debugC(kDebugResource, "\trect10.right: %d", _rect10.right);
	debugC(kDebugResource, "\trect10.top: %d", _rect10.top);
	debugC(kDebugResource, "\trect10.bottom: %d", _rect10.bottom);
	debugC(kDebugResource, "\tu0: %d", _u0);
	debugC(kDebugResource, "\tu1: %d", _u1);
	debugC(kDebugResource, "\t_mouseDownOpcode: %d", _mouseDownOpcode);
	debugC(kDebugResource, "\t_mouseDragOpcode: %d", _mouseDragOpcode);
	debugC(kDebugResource, "\t_mouseUpOpcode: %d", _mouseUpOpcode);

	// TODO: Think that u0 and u1 are unused in Type 10
	if (_u0)
		warning("Type 10 u0 non-zero");
	if (_u1)
		warning("Type 10 u1 non-zero");

	// TODO: Not sure about order of Mouse Down, Mouse Drag and Mouse Up
	//       Or whether this is slightly different...
	debugCN(kDebugResource, "Type 10 _mouseDownOpcode: %d\n", _mouseDownOpcode);
	debugCN(kDebugResource, "Type 10 _mouseDragOpcode: %d\n", _mouseDragOpcode);
	debugCN(kDebugResource, "Type 10 _mouseUpOpcode: %d\n", _mouseUpOpcode);

	for (byte i = 0; i < 4; i++) {
		debugC(kDebugResource, "\tList %d:", i);

		_lists[i].listCount = rlstStream->readUint16LE();
		debugC(kDebugResource, "\t%d values", _lists[i].listCount);

		_lists[i].list = new uint16[_lists[i].listCount];
		for (uint16 j = 0; j < _lists[i].listCount; j++) {
			_lists[i].list[j] = rlstStream->readUint16LE();
			debugC(kDebugResource, "\tValue %d: %d", j, _lists[i].list[j]);
		}
	}

	_mouseDown = false;
	warning("TODO: Card contains Type 10 Resource - Function not yet implemented");
}

MystResourceType10::~MystResourceType10() {
	for (byte i = 0; i < 4; i++)
		delete[] _lists[i].list;
}

void MystResourceType10::handleMouseDown() {
	_mouseDown = true;

	_vm->_scriptParser->runOpcode(_mouseDownOpcode);
}

void MystResourceType10::handleMouseUp() {
	_mouseDown = false;

	_vm->_scriptParser->runOpcode(_mouseUpOpcode);
}

void MystResourceType10::handleMouseMove() {
	if (_mouseDown) {
		_vm->_scriptParser->runOpcode(_mouseDragOpcode);
	}
}

MystResourceType11::MystResourceType11(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResourceType8(vm, rlstStream, parent) {
	_kind = rlstStream->readUint16LE();
	// NOTE: l,r,t,b differs from standard l,t,r,b order
	_rect11.left = rlstStream->readUint16LE();
	_rect11.right = rlstStream->readUint16LE();
	_rect11.top = rlstStream->readUint16LE();
	_rect11.bottom = rlstStream->readUint16LE();
	_u0 = rlstStream->readUint16LE();
	_u1 = rlstStream->readUint16LE();
	_mouseDownOpcode = rlstStream->readUint16LE();
	_mouseDragOpcode = rlstStream->readUint16LE();
	_mouseUpOpcode = rlstStream->readUint16LE();

	debugC(kDebugResource, "\tkind: %d", _kind);
	debugC(kDebugResource, "\trect11.left: %d", _rect11.left);
	debugC(kDebugResource, "\trect11.right: %d", _rect11.right);
	debugC(kDebugResource, "\trect11.top: %d", _rect11.top);
	debugC(kDebugResource, "\trect11.bottom: %d", _rect11.bottom);
	debugC(kDebugResource, "\tu0: %d", _u0);
	debugC(kDebugResource, "\tu1: %d", _u1);
	debugC(kDebugResource, "\t_mouseDownOpcode: %d", _mouseDownOpcode);
	debugC(kDebugResource, "\t_mouseDragOpcode: %d", _mouseDragOpcode);
	debugC(kDebugResource, "\t_mouseUpOpcode: %d", _mouseUpOpcode);

	// TODO: Think that u0 and u1 are unused in Type 11
	if (_u0)
		warning("Type 11 u0 non-zero");
	if (_u1)
		warning("Type 11 u1 non-zero");

	// TODO: Not sure about order of Mouse Down, Mouse Drag and Mouse Up
	//       Or whether this is slightly different...
	debugCN(kDebugResource, "Type 11 _mouseDownOpcode: %d\n", _mouseDownOpcode);
	debugCN(kDebugResource, "Type 11 _mouseDragOpcode: %d\n", _mouseDragOpcode);
	debugCN(kDebugResource, "Type 11 _mouseUpOpcode: %d\n", _mouseUpOpcode);

	for (byte i = 0; i < 3; i++) {
		debugC(kDebugResource, "\tList %d:", i);

		_lists[i].listCount = rlstStream->readUint16LE();
		debugC(kDebugResource, "\t%d values", _lists[i].listCount);

		_lists[i].list = new uint16[_lists[i].listCount];
		for (uint16 j = 0; j < _lists[i].listCount; j++) {
			_lists[i].list[j] = rlstStream->readUint16LE();
			debugC(kDebugResource, "\tValue %d: %d", j, _lists[i].list[j]);
		}
	}

	_mouseDown = false;
}

MystResourceType11::~MystResourceType11() {
	for (byte i = 0; i < 3; i++)
		delete[] _lists[i].list;
}

void MystResourceType11::handleMouseDown() {
	_mouseDown = true;

	_vm->_scriptParser->runOpcode(_mouseDownOpcode);
}

void MystResourceType11::handleMouseUp() {
	_mouseDown = false;

	_vm->_scriptParser->runOpcode(_mouseUpOpcode);
}

void MystResourceType11::handleMouseMove() {
	if (_mouseDown) {
		_vm->_scriptParser->runOpcode(_mouseDragOpcode);
	}
}

MystResourceType12::MystResourceType12(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResourceType8(vm, rlstStream, parent) {
	_kind = rlstStream->readUint16LE();
	// NOTE: l,r,t,b differs from standard l,t,r,b order
	_rect11.left = rlstStream->readUint16LE();
	_rect11.right = rlstStream->readUint16LE();
	_rect11.top = rlstStream->readUint16LE();
	_rect11.bottom = rlstStream->readUint16LE();
	_state0Frame = rlstStream->readUint16LE();
	_state1Frame = rlstStream->readUint16LE();
	_mouseDownOpcode = rlstStream->readUint16LE();
	_mouseDragOpcode = rlstStream->readUint16LE();
	_mouseUpOpcode = rlstStream->readUint16LE();

	debugC(kDebugResource, "\tkind: %d", _kind);
	debugC(kDebugResource, "\trect11.left: %d", _rect11.left);
	debugC(kDebugResource, "\trect11.right: %d", _rect11.right);
	debugC(kDebugResource, "\trect11.top: %d", _rect11.top);
	debugC(kDebugResource, "\trect11.bottom: %d", _rect11.bottom);
	debugC(kDebugResource, "\t_state0Frame: %d", _state0Frame);
	debugC(kDebugResource, "\t_state1Frame: %d", _state1Frame);
	debugC(kDebugResource, "\t_mouseDownOpcode: %d", _mouseDownOpcode);
	debugC(kDebugResource, "\t_mouseDragOpcode: %d", _mouseDragOpcode);
	debugC(kDebugResource, "\t_mouseUpOpcode: %d", _mouseUpOpcode);

	// TODO: Think that u0 and u1 are animation frames to be
	//       drawn for var == 0 and var == 1
	debugCN(kDebugResource, "Type 12 _state0Frame: %d\n", _state0Frame);
	debugCN(kDebugResource, "Type 12 _state1Frame: %d\n", _state1Frame);

	// TODO: Not sure about order of Mouse Down, Mouse Drag and Mouse Up
	//       Or whether this is slightly different...
	debugCN(kDebugResource, "Type 12 _mouseDownOpcode: %d\n", _mouseDownOpcode);
	debugCN(kDebugResource, "Type 12 _mouseDragOpcode: %d\n", _mouseDragOpcode);
	debugCN(kDebugResource, "Type 12 _mouseUpOpcode: %d\n", _mouseUpOpcode);

	for (byte i = 0; i < 3; i++) {
		debugC(kDebugResource, "\tList %d:", i);

		_lists[i].listCount = rlstStream->readUint16LE();
		debugC(kDebugResource, "\t%d values", _lists[i].listCount);

		_lists[i].list = new uint16[_lists[i].listCount];
		for (uint16 j = 0; j < _lists[i].listCount; j++) {
			_lists[i].list[j] = rlstStream->readUint16LE();
			debugC(kDebugResource, "\tValue %d: %d", j, _lists[i].list[j]);
		}
	}

	warning("TODO: Card contains Type 12, Type 11 section Resource - Function not yet implemented");

	_numFrames = rlstStream->readUint16LE();
	_firstFrame = rlstStream->readUint16LE();
	uint16 frameWidth = rlstStream->readUint16LE();
	uint16 frameHeight = rlstStream->readUint16LE();
	_frameRect.left = rlstStream->readUint16LE();
	_frameRect.top = rlstStream->readUint16LE();

	_frameRect.right = _frameRect.left + frameWidth;
	_frameRect.bottom = _frameRect.top + frameHeight;

	debugC(kDebugResource, "\t_numFrames: %d", _numFrames);
	debugC(kDebugResource, "\t_firstFrame: %d", _firstFrame);
	debugC(kDebugResource, "\tframeWidth: %d", frameWidth);
	debugC(kDebugResource, "\tframeHeight: %d", frameHeight);
	debugC(kDebugResource, "\t_frameRect.left: %d", _frameRect.left);
	debugC(kDebugResource, "\t_frameRect.top: %d", _frameRect.top);
	debugC(kDebugResource, "\t_frameRect.right: %d", _frameRect.right);
	debugC(kDebugResource, "\t_frameRect.bottom: %d", _frameRect.bottom);

	_doAnimation = false;
}

MystResourceType12::~MystResourceType12() {
	for (byte i = 0; i < 3; i++)
		delete[] _lists[i].list;
}

void MystResourceType12::handleAnimation() {
	// TODO: Probably not final version. Variable/Type 11 Controlled?
	if (_doAnimation) {
		_vm->_gfx->copyImageToScreen(_currentFrame++, _frameRect);
		if ((_currentFrame - _firstFrame) >= _numFrames)
			_doAnimation = false;
	}
}

void MystResourceType12::handleMouseUp() {
	// HACK/TODO: Trigger Animation on Mouse Click. Probably not final version. Variable/Type 11 Controlled?
	_currentFrame = _firstFrame;
	_doAnimation = true;
}

MystResourceType13::MystResourceType13(MohawkEngine_Myst *vm, Common::SeekableReadStream *rlstStream, MystResource *parent) : MystResource(vm, rlstStream, parent) {
	_enterOpcode = rlstStream->readUint16LE();
	_leaveOpcode = rlstStream->readUint16LE();

	debugC(kDebugResource, "\t_enterOpcode: %d", _enterOpcode);
	debugC(kDebugResource, "\t_leaveOpcode: %d", _leaveOpcode);
}

void MystResourceType13::handleMouseEnter() {
	// Pass along the enter opcode (with no parameters) to the script parser
	_vm->_scriptParser->runOpcode(_enterOpcode);
}

void MystResourceType13::handleMouseLeave() {
	// Pass along the leave opcode (with no parameters) to the script parser
	_vm->_scriptParser->runOpcode(_leaveOpcode);
}

void MystResourceType13::handleMouseUp() {
	// Type 13 Resources do nothing on Mouse Clicks.
	// This is required to override the inherited default
	// i.e. MystResource::handleMouseUp
}

} // End of namespace Mohawk