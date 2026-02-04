#pragma once
#include "CameraController.h"
#include "DeathParticles.h"
#include "Fade.h"
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Method.h"
#include "enemy/Enemy.h"
#include "goal/Goal.h"
#include "platform/CrumblePlatform.h"
#include "player/Player.h"
#include "player/PlayerBullet.h"
#include "player/WireBlock.h"
#include "skydome/Skydome.h"
#include <memory>
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

	void GenerateBlocks();

	// すべての当たり判定を行う
	void CheckAllCollisions();

	void ChangePhase();

	// デスフラグのgetter
	bool IsFinished() const { return finished_; };

	// ★追加：結果種別
	enum class Result { kNone, kClear, kFailed };
	Result GetResult() const { return result_; } // ゲッター

private:
	void UpdateBullets_();
	void RemoveDeadEnemies_();

	// オートエイム：射撃ターゲット取得（見つからなければ false）
	bool TryGetShootTargetPos_(Vector3& outPos) const;

private:
	// カメラ
	Camera camera_;
	// 3DPlayerモデルデータ
	Model* playerModel_ = nullptr;
	// 3DEnemyモデルデータ
	Model* enemyModel_ = nullptr;
	// 3DEnemyモデルデータ
	Model* particleModel_ = nullptr;
	// ブロックモデルデータ
	Model* modelBlock_ = nullptr;
	// 天球モデルデータ
	Model* modelSkydome_ = nullptr;
	// ダメージブロックモデル
	Model* modelDamageBlock_ = nullptr;
	// 梯子モデル
	Model* modelLadder_ = nullptr;
	// 弾モデル
	Model* bulletModel_ = nullptr;

	// プレイヤーの弾
	std::vector<std::unique_ptr<PlayerBullet>> bullets_;

	// 梯子のTransform（保持して使い回す）
	std::vector<WorldTransform*> ladderWorldTransforms_;

	// ダメージブロックのTransform
	std::vector<std::vector<WorldTransform*>> worldTransformDamageBlocks_;

	bool isDebugCameraActive_ = false;
	// デバッグカメラ
	DebugCamera* debugCamera_ = nullptr;

	// 自キャラ
	Player* player_ = nullptr;
	// 敵キャラ
	std::list<Enemy*> enemies_;
	// 天球
	Skydome* skydome_ = nullptr;

	Model* wireBlockModel_ = nullptr;
	Model* wireCircleModel_ = nullptr;
	WireBlock wireViz_;
	// ワイヤー可視化のロック（確定ターゲット）
	bool wireVizLocked_ = false;
	Vector3 wireLockedTarget_{0.0f, 0.0f, 0.0f};
	bool prevPlayerWiring_ = false;

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

	std::vector<std::vector<WorldTransform*>> worldTransformBlocks_;

	Fade* fade_ = nullptr;

	// 置き換え：直置きのTransform/AABBではなくクラスを持つ
	Goal* goal_ = nullptr;

	// ゲーム結果
	Result result_ = Result::kNone;

	Sprite* moveSprite_ = nullptr;
	uint32_t textureHandle_ = 0;

	bool playedDeathSe_ = false;
	bool playedGoalSe_ = false;

	// 崩れる床
	std::vector<CrumblePlatform*> crumblePlatforms_;

	// 連射
	float shootCooldown_ = 0.0f;
	static inline const float kShootIntervalSec = 0.12f; // 連射間隔（調整用）

	// ---- Ammo UI ----
	static inline const int kDigitCount = 10;
	uint32_t digitTex_[kDigitCount]{};

	Sprite* ammoCurDigits_[kDigitCount]{}; // 0-9
	Sprite* ammoMaxDigits_[kDigitCount]{}; // 0-9
	Sprite* ammoSlashSprite_ = nullptr;

	uint32_t slashTex_ = 0; // "/" 用
};
