#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

struct AABB {
	Vector3 min; // 最小点
	Vector3 max; // 最大点
};

Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
Matrix4x4 MatrixMultiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 MakeAffineMatrix(Vector3 scale, Vector3 rotate, Vector3 translate);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
Vector3 Lerp(const Vector3& v1, const Vector3& v2, float t);
float EaseInOut(float t);
bool IsCollision(const AABB& a, const AABB& b);
Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);
float Length(const Vector3& v);
Vector3 Normalize(const Vector3& v);
// ★追加：AABB同士の交差判定
bool IntersectAABB(const AABB& a, const AABB& b);