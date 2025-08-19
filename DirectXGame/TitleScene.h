#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

class TitleScene {
public:
	void Initialize();
	void Update();
	void Draw();

	// デスフラグのgetter
	bool IsFinished() const { return finished_; }
private:
	// 終了フラグ
	bool finished_ = false;

};
