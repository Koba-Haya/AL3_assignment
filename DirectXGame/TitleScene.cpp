#include "TitleScene.h"

using namespace KamataEngine;

namespace {
constexpr float kFadeTimeSec = 1.0f;
}

TitleScene::~TitleScene() {
	delete fade_;
	fade_ = nullptr;
	delete titleSprite_;
	titleSprite_ = nullptr;
}

void TitleScene::Initialize(bool returningFromGame) {
	finished_ = false;

	fade_ = new Fade();
	fade_->Initialize();

	if (returningFromGame) {
		// ★ゲームから戻った直後は黒→透明のフェードイン
		fade_->Start(Fade::Status::FadeIn, kFadeTimeSec);
		phase_ = Phase::kFadeIn;
	} else {
		// ★初回は透明で静止（フェードなし）
		phase_ = Phase::kMain;
	}

	textureHandle_ = TextureManager::Load("scene/title.png");
	// ★重要：sprite_->Create(...) ではなく Sprite::Create(...) で生成
	titleSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f});
	titleSprite_->SetSize(Vector2(1280.0f, 720.0f));
}

void TitleScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop(); // 描画コスト削減
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain: {
		auto* input = Input::GetInstance();
		const bool pressedSpace = input->TriggerKey(DIK_SPACE) || input->PushKey(DIK_SPACE);
		if (pressedSpace) {
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);
			phase_ = Phase::kFadeOut;
		}
	} break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			// ★真っ黒になったので遷移OK
			finished_ = true;
			// 停止しない：このフレームは黒を保ったまま、次フレーム先頭でシーン切替
		}
		break;
	}
}

void TitleScene::Draw() {
	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
	titleSprite_->Draw();
	Sprite::PostDraw();
	// タイトル本体の描画があればここに…
	if (fade_) {
		fade_->Draw();
	}
}
