#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Enemy.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Method.h"
#include "Player.h"
#include "Skydome.h"
#include "Fade.h"
#include "Goal.h"
#include <vector>

using namespace KamataEngine;

class GameScene {
public:
	// デストラクタ
	~GameScene();

	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

	void GenetateBlocks();

	// すべての当たり判定を行う
	void CheckAllCollisions();

	void ChangePhase();

	// デスフラグのgetter
	bool IsFinished() const { return finished_; };

	// ★追加：結果種別
	enum class Result { kNone, kClear, kFailed };
	Result GetResult() const { return result_; } // ゲッター
private:
	// カメラ
	KamataEngine::Camera camera_;
	// 3DPlayerモデルデータ
	KamataEngine::Model* playerModel_ = nullptr;
	// 3DEnemyモデルデータ
	KamataEngine::Model* enemyModel_ = nullptr;
	// 3DEnemyモデルデータ
	KamataEngine::Model* particleModel_ = nullptr;
	// ブロックモデルデータ
	KamataEngine::Model* modelBlock_ = nullptr;
	// 天球モデルデータ
	KamataEngine::Model* modelSkydome_ = nullptr;

	bool isDebugCameraActive_ = false;
	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// 自キャラ
	Player* player_ = nullptr;

	// 天球
	Skydome* skydome_ = nullptr;

	// 敵キャラ
	std::list<Enemy*> enemies_;

	// マップチップフィールド
	MapChipField* mapChipField_;

	// カメラコントローラー
	CameraController* cameraController_ = nullptr;

	DeathParticles* deathParticles_ = nullptr;

	enum class Phase {
		kFadeIn,  // 開始時フェードイン
		kPlay,    // プレイ中
		kDeath,   // （任意）死亡演出など
		kFadeOut, // （任意）他シーンへ
	};

	// ゲームの現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// 終了フラグ
	bool finished_ = false;

	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	Fade* fade_ = nullptr;

    // ★置き換え：直置きのTransform/AABBではなくクラスを持つ
	Goal* goal_ = nullptr;

	// ★追加：ゲーム結果
	Result result_ = Result::kNone;

	Sprite* moveSprite_ = nullptr;
	uint32_t textureHandle_ = 0;
};
