#define NOMINMAX

#include "GameScene.h"
#include "SoundManager.h"
#include <Windows.h>
#include <cmath>

using namespace KamataEngine;

namespace {
constexpr float kFadeTimeSec = 1.0f;
constexpr int kScreenW = 1280;
constexpr int kScreenH = 720;

// 左クリックのトリガー（押した瞬間）
static bool IsLeftClickTriggered() {
	static SHORT prev = 0;
	SHORT now = GetAsyncKeyState(VK_LBUTTON);
	bool triggered = ((now & 0x8000) != 0) && ((prev & 0x8000) == 0);
	prev = now;
	return triggered;
}

// クライアント座標のマウス位置を取る
static POINT GetMouseClientPos() {
	POINT p{};
	GetCursorPos(&p);
	HWND hwnd = GetActiveWindow();
	ScreenToClient(hwnd, &p);
	return p;
}

// 画面座標→ワールドレイ（DirectXのNDC: zは0..1）
static void BuildPickRay(const Camera& cam, int mouseX, int mouseY, Vector3& outOrigin, Vector3& outDir) {
	float nx = (2.0f * mouseX) / float(kScreenW) - 1.0f;
	float ny = 1.0f - (2.0f * mouseY) / float(kScreenH);

	// NDC空間の近点/遠点
	Vector3 ndcNear = {nx, ny, 0.0f};
	Vector3 ndcFar = {nx, ny, 1.0f};

	Matrix4x4 vp = MatrixMultiply(cam.matView, cam.matProjection);
	Matrix4x4 invVP = Inverse(vp);

	Vector3 worldNear = Transform(ndcNear, invVP);
	Vector3 worldFar = Transform(ndcFar, invVP);

	outOrigin = worldNear;
	Vector3 d = {worldFar.x - worldNear.x, worldFar.y - worldNear.y, worldFar.z - worldNear.z};
	outDir = Normalize(d);
}

// レイと平面(z=0)の交点を求める（ゲームがほぼz=0で動くのでこれでOK）
static bool RayPlaneZ0(const Vector3& origin, const Vector3& dir, Vector3& outHit) {
	const float denom = dir.z;
	if (std::fabs(denom) < 1e-6f) {
		return false;
	}
	float t = (0.0f - origin.z) / denom;
	if (t < 0.0f) {
		return false;
	}
	outHit = {origin.x + dir.x * t, origin.y + dir.y * t, 0.0f};
	return true;
}
} // namespace

// static inline bool IntersectAABB(const AABB& a, const AABB& b) {
//	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
// }

GameScene::~GameScene() {
	// 3Dモデルデータの開放
	delete playerModel_;
	// 3Dモデルデータの開放
	delete enemyModel_;
	// デスパーティクルモデルの開放
	delete particleModel_;
	// ブロックモデルデータの開放
	delete modelBlock_;
	// 天球モデルデータの開放
	delete modelSkydome_;
	// デバッグカメラの開放
	delete debugCamera_;
	// 自キャラの開放
	delete player_;
	// 天球の開放
	delete skydome_;
	// デスパーティクルの開放
	delete deathParticles_;
	// フェードの開放
	delete fade_;
	// ゴールの開放
	delete goal_;
	goal_ = nullptr;
	// 敵キャラの開放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// マップチップフィールドの開放
	delete mapChipField_;

	// ブロックモデルの開放
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}
	worldTransformBlocks_.clear();

	// ダメージブロックモデルの開放
	delete modelDamageBlock_;
	modelDamageBlock_ = nullptr;

	// ダメージブロックのワールド変換データの開放
	for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
		for (WorldTransform* wt : line) {
			delete wt;
		}
	}
	worldTransformDamageBlocks_.clear();
}

void GameScene::Initialize() {

	// 3Dモデルデータの生成
	playerModel_ = Model::CreateFromOBJ("player", true);
	// 3Dモデルデータの生成
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	// デスパーティクルモデルの生成
	particleModel_ = Model::CreateFromOBJ("particle", true);
	// ブロックモデルデータの生成
	modelBlock_ = Model::CreateFromOBJ("cube", true);
	// 天球モデルデータの生成
	modelSkydome_ = Model::CreateFromOBJ("sky_sphere", true);
	// ダメージブロックモデル
	modelDamageBlock_ = Model::CreateFromOBJ("damageBlock", true);

	// カメラのfarZを適度に大きい値に
	camera_.farZ = 1000.0f;
	// カメラの初期化
	camera_.Initialize();
	camera_.translation_.z = -30.0f;

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// マップチップフィールドの生成
	mapChipField_ = new MapChipField;
	// CSVファイルからマップデータを読み込み
	mapChipField_->LoadMapChipCsv("Resources/block.csv");

	// 自キャラの生成
	player_ = new Player;
	// 座標をマップチップ番号で指定
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);
	// 自キャラの初期化
	player_->Initialize(playerModel_, &camera_, playerPosition);
	// マップチップデータのセット
	player_->SetMapChipField(mapChipField_);

	// 敵を複数生成
	const int enemyCount = 3;
	for (int32_t i = 0; i < enemyCount; ++i) {
		Enemy* newEnemy = new Enemy();

		// 例：プレイヤーと同じ「地面の1つ上の段」に置く
		// xIndex は適当にずらす（例: 6, 10, 14）
		Vector3 enemyPosition;
		if (i == 0) {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(6, 14);
		} else if (i == 1) {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(14, 18);
		} else {
			enemyPosition = mapChipField_->GetMapChipPositionByIndex(18, 18);
		}

		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);

		newEnemy->SetMapChipField(mapChipField_);
		enemies_.push_back(newEnemy);
	}

	// 天球の生成
	skydome_ = new Skydome;
	// 天球の初期化
	skydome_->Initialize();

	GenerateBlocks();

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

	Model* goalModel = Model::CreateFromOBJ("goal", true);
	Vector3 goalPos = mapChipField_->GetMapChipPositionByIndex(30, 18);

	goal_ = new Goal();
	goal_->Initialize(goalModel, goalPos);

	// フェード
	fade_ = new Fade();
	fade_->Initialize();

	// ★ゲーム開始時は黒→透明のフェードイン。完了までは動かさない
	fade_->Start(Fade::Status::FadeIn, kFadeTimeSec);
	phase_ = Phase::kFadeIn;

	// カメラコントローラーの初期化
	cameraController_ = new CameraController; // 生成
	cameraController_->Initialize();          // 初期化
	cameraController_->SetTarget(player_);    // 追従対象をセット
	cameraController_->Reset();               // リセット（瞬間合わせ）

	textureHandle_ = TextureManager::Load("scene/move.png");
	// ★重要：sprite_->Create(...) ではなく Sprite::Create(...) で生成
	moveSprite_ = Sprite::Create(textureHandle_, {0.0f, 0.0f});
	moveSprite_->SetSize(Vector2(1280.0f, 720.0f));

	// ワイヤービジュアライザー用モデルの生成
	wireBlockModel_ = Model::CreateFromOBJ("block", true);

	// ワイヤー目標円モデルの生成
	wireCircleModel_ = Model::CreateFromOBJ("circle", true);

	wireViz_.Initialize(64);
	wireViz_.SetMaxLength(12.0f);
	wireViz_.SetBlockSpacing(0.6f);
	wireViz_.SetGroundOffsetY(0.02f);

	wireViz_.SetCircleScale({1.0f, 1.0f, 1.0f});
	// ブロックのサイズ
	wireViz_.SetBlockScale({0.2f, 0.2f, 0.2f});

	playedDeathSe_ = false;
	playedGoalSe_ = false;

	SoundManager::Instance().PlayBgmGame(0.25f);
}

void GameScene::Update() {
	ChangePhase();

	switch (phase_) {
	case Phase::kFadeIn:

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				// アフィン変換行列の作成
				Matrix4x4 blockAffineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
				// ワールド行列に代入
				worldTransformBlock->matWorld_ = blockAffineMatrix;
				// 定数バッファの転送
				worldTransformBlock->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		if (player_) {
			player_->UpdateFreeze();
		}
		for (auto* e : enemies_) {
			e->UpdateFreeze();
		}

		fade_->Update();
		if (fade_->IsFinished()) {
			fade_->Stop(); // 完了したら止めて描画コスト削減
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay: {

		if (goal_) {
			goal_->Update();
		}

		// ワイヤー状態の変化を検出して、終了したらロック解除
		const bool nowWiring = (player_ && player_->IsWiring());
		if (prevPlayerWiring_ && !nowWiring) {
			wireVizLocked_ = false;
		}
		prevPlayerWiring_ = nowWiring;

		// 左クリック（押した瞬間）でターゲット確定＆開始
		if (player_ && IsLeftClickTriggered() && !player_->IsWiring()) {
			POINT mp = GetMouseClientPos();

			Vector3 rayO{}, rayD{};
			BuildPickRay(camera_, mp.x, mp.y, rayO, rayD);

			Vector3 clickedWorld{};
			if (RayPlaneZ0(rayO, rayD, clickedWorld)) {
				const Vector3 playerPos = player_->GetWorldTransform().translation_;
				Vector3 to = {clickedWorld.x - playerPos.x, clickedWorld.y - playerPos.y, clickedWorld.z - playerPos.z};

				float len = Length(to);
				if (len > 1e-6f) {
					Vector3 dir = Normalize(to);

					const float maxDist = 12.0f;
					float useDist = std::min(len, maxDist);

					Vector3 target = {playerPos.x + dir.x * useDist, playerPos.y + dir.y * useDist, playerPos.z + dir.z * useDist};

					const bool before = player_->IsWiring();
					player_->StartWire(target);
					const bool after = player_->IsWiring();

					if (!before && after) {
						SoundManager::Instance().PlaySeWireMove(0.75f);
						wireVizLocked_ = true;
						wireLockedTarget_ = target;
					}
				}
			}
		}

		// 可視化更新
		if (player_) {
			const Vector3 playerPos = player_->GetWorldTransform().translation_;

			if (wireVizLocked_ && player_->IsWiring()) {
				// 移動中は固定
				wireViz_.Update(playerPos, wireLockedTarget_);
			} else {
				// 移動してないなら、使える時だけプレビュー更新
				if (player_->CanUseWire()) {
					POINT mp = GetMouseClientPos();
					Vector3 rayO{}, rayD{};
					BuildPickRay(camera_, mp.x, mp.y, rayO, rayD);

					Vector3 cursorWorld{};
					if (RayPlaneZ0(rayO, rayD, cursorWorld)) {
						wireViz_.Update(playerPos, cursorWorld);
					}
				}
				// 使えないなら更新しない（前フレームの値が残るのが嫌なら Hide を使うが、今回は Draw 条件で消すのでOK）
			}
		}

		// 自キャラ更新など既存処理
		player_->Update();
		skydome_->Update();
		for (Enemy* enemy : enemies_) {
			enemy->Update();
		}

#ifdef _DEBUG
		if (Input::GetInstance()->TriggerKey(DIK_1)) { // 例：キー1で切り替え
			isDebugCameraActive_ = !isDebugCameraActive_;
		}
#endif

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 m = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = m;
				wt->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		// すべての当たり判定を行う
		CheckAllCollisions();

		// 死亡検知→フェードアウト開始（タイトルへ戻る準備）
		if (player_->IsDead()) {

			if (!playedDeathSe_) {
				playedDeathSe_ = true;
				SoundManager::Instance().PlaySePlayerDeath(0.95f);
			}

			const Vector3& pos = player_->GetWorldPosition();
			deathParticles_ = new DeathParticles;
			deathParticles_->Initialize(particleModel_, &camera_, pos);
			phase_ = Phase::kDeath;
			return;
		}

		// ゴール到達判定（通り抜けOKのトリガー）
		if (goal_ && goal_->IsActive() && IntersectAABB(player_->GetAABB(), goal_->GetAABB())) {
			if (!playedGoalSe_) {
				playedGoalSe_ = true;
				SoundManager::Instance().PlaySeGoal(0.95f);
			}

			goal_->SetActive(false);
			result_ = Result::kClear;
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);
			phase_ = Phase::kFadeOut;
			break;
		}

		break;
	}

	case Phase::kDeath:

		// パーティクルの更新
		if (deathParticles_) {
			deathParticles_->Update();
		}

		if (isDebugCameraActive_) {
			debugCamera_->Update();
			// DebugCamera から Camera を取得し、camera_ にコピー
			camera_.matView = debugCamera_->GetCamera().matView;
			camera_.matProjection = debugCamera_->GetCamera().matProjection;

			camera_.TransferMatrix();
		} else {
			// カメラコントローラーの更新
			cameraController_->Update();

			// カメラを controller から取得して camera_ に反映
			const Camera& controlledCam = cameraController_->GetCamera();
			camera_.matView = controlledCam.matView;
			camera_.matProjection = controlledCam.matProjection;

			// ここで行列転送も必要（たとえば TransferMatrix などが必要なら）
			camera_.TransferMatrix();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				// アフィン変換行列の作成
				Matrix4x4 blockAffineMatrix = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);
				// ワールド行列に代入
				worldTransformBlock->matWorld_ = blockAffineMatrix;
				// 定数バッファの転送
				worldTransformBlock->TransferMatrix();
			}
		}
		// ダメージブロックの更新（matWorld_/CB転送）
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				Matrix4x4 affine = MakeAffineMatrix(wt->scale_, wt->rotation_, wt->translation_);
				wt->matWorld_ = affine;
				wt->TransferMatrix();
			}
		}

		if (deathParticles_ && deathParticles_->IsFinished()) {
			result_ = Result::kFailed;

			// 画面フェード（透明→黒）
			fade_->Start(Fade::Status::FadeOut, kFadeTimeSec);

			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:

		// ★フェードアウト中は基本停止。必要なら背景だけUpdateしてもOK
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true; // → タイトルへ
		}
		break;

	default:
		break;
	}
}

void GameScene::Draw() {
	switch (phase_) {

	case Phase::kFadeIn:
		// プレイヤーの描画
		player_->Draw();
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;

	case Phase::kPlay: {

		// 自キャラの描画
		player_->Draw();
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}

		const bool canDrawWireViz = (player_ && (player_->IsWiring() || player_->CanUseWire()));
		if (canDrawWireViz) {
			// ブロック列
			if (wireBlockModel_) {
				const auto& wts = wireViz_.GetLineBlockWTs();
				const int count = wireViz_.GetActiveBlockCount();
				for (int i = 0; i < count; ++i) {
					auto& wtPtr = wts[static_cast<size_t>(i)];
					if (wtPtr) {
						wireBlockModel_->Draw(*wtPtr, camera_);
					}
				}
			}

			// 最終地点（円）
			if (wireCircleModel_) {
				wireCircleModel_->Draw(wireViz_.GetTargetCircleWT(), camera_);
			}
		}

		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		break;
	}
	case Phase::kDeath: {

		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		if (deathParticles_) {
			deathParticles_->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		break;
	}
	case Phase::kFadeOut: {
		// 天球の描画
		skydome_->Draw(&camera_);
		// 敵キャラの描画
		for (Enemy* enemy : enemies_) {
			enemy->Draw();
		}
		// ブロックの描画
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock)
					continue;
				modelBlock_->Draw(*worldTransformBlock, camera_);
			}
		}
		// ダメージブロックの描画
		for (std::vector<WorldTransform*>& line : worldTransformDamageBlocks_) {
			for (WorldTransform* wt : line) {
				if (!wt)
					continue;
				modelDamageBlock_->Draw(*wt, camera_);
			}
		}

		// ゴール描画
		if (goal_)
			goal_->Draw(camera_);
		Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList());
		moveSprite_->Draw();
		Sprite::PostDraw();
		// フェードは最後に
		fade_->Draw();
		break;
	}
	default:
		break;
	}
}

void GameScene::GenerateBlocks() {
	const uint32_t kNumBlockVirtical = mapChipField_->GetNumBlockVertical();
	const uint32_t kNumBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	worldTransformBlocks_.clear();
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	}

	worldTransformDamageBlocks_.clear();
	worldTransformDamageBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		worldTransformDamageBlocks_[i].resize(kNumBlockHorizontal);
	}

	for (uint32_t i = 0; i < kNumBlockVirtical; ++i) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {

			MapChipType type = mapChipField_->GetMapChipTypeByIndex(j, i);

			if (type == MapChipType::kBlock) {
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformBlocks_[i][j] = wt;
			} else if (type == MapChipType::kDamage) {
				WorldTransform* wt = new WorldTransform();
				wt->Initialize();
				wt->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				worldTransformDamageBlocks_[i][j] = wt;
			}
		}
	}
}

// タイルRect→AABBにして判定するユーティリティ
static AABB MakeTileAabbFromRect(const Rect& r) {
	AABB a;
	a.min = {r.left, r.bottom, -0.5f};
	a.max = {r.right, r.top, +0.5f};
	return a;
}

void GameScene::CheckAllCollisions() {
	{
		AABB aabb1 = player_->GetAABB();
		for (Enemy* enemy : enemies_) {
			AABB aabb2 = enemy->GetAABB();
			if (IsCollision(aabb1, aabb2)) {
				player_->OnCollision(enemy);
				enemy->OnCollision(player_);
			}
		}
	}

	// ダメージタイル衝突（4隅＋中心）
	{
		const AABB playerAabb = player_->GetAABB();

		// 5点サンプル（4隅＋中心）
		Vector3 pts[5] = {
		    playerAabb.min,
		    {playerAabb.max.x,                             playerAabb.min.y,                             playerAabb.min.z                            },
		    {playerAabb.min.x,                             playerAabb.max.y,                             playerAabb.min.z                            },
		    playerAabb.max,
		    {(playerAabb.min.x + playerAabb.max.x) * 0.5f, (playerAabb.min.y + playerAabb.max.y) * 0.5f, (playerAabb.min.z + playerAabb.max.z) * 0.5f}
        };

		// 重複タイルを避けるための簡易集合（最大5個なのでベタでOK）
		IndexSet ids[5]{};
		int idCount = 0;

		auto addUnique = [&](IndexSet v) {
			for (int i = 0; i < idCount; ++i) {
				if (ids[i].xIndex == v.xIndex && ids[i].yIndex == v.yIndex)
					return;
			}
			ids[idCount++] = v;
		};

		for (int i = 0; i < 5; ++i) {
			IndexSet idx = mapChipField_->GetMapChipIndexSetByPosition(pts[i]);
			// 範囲外は GetMapChipTypeByIndex が blank を返す想定だけど、
			// ここで弾くなら弾いてもOK
			addUnique(idx);
		}

		for (int i = 0; i < idCount; ++i) {
			const uint32_t x = ids[i].xIndex;
			const uint32_t y = ids[i].yIndex;

			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kDamage) {
				continue;
			}

			Rect r = mapChipField_->GetRectByIndex(x, y);
			AABB tileAabb = MakeTileAabbFromRect(r);

			if (IsCollision(playerAabb, tileAabb)) {
				player_->OnCollision(nullptr);
				return;
			}
		}
	}
}

void GameScene::ChangePhase() {
	switch (phase_) {
	case Phase::kPlay:
		if (player_->IsDead()) {
			// 死亡演出フェーズに切り替え
			phase_ = Phase::kDeath;
			// 自キャラの座標を取得
			// const Vector3& deathParticlesPosition = player_->GetWorldPosition();
		}
		break;
	case Phase::kDeath:
		break;
	default:
		break;
	}
}
