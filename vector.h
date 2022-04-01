
#include <numbers>
#include <cmath>

struct vector3
{
	// constructor
	constexpr vector3(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f) noexcept :
		x(x), y(y), z(z) { }

	// operator overloads
	constexpr const vector3& operator-(const vector3& other) const noexcept
	{
		return vector3{ x - other.x, y - other.y, z - other.z };
	}

	constexpr const vector3& operator+(const vector3& other) const noexcept
	{
		return vector3{ x + other.x, y + other.y, z + other.z };
	}

	constexpr const vector3& operator/(const float factor) const noexcept
	{
		return vector3{ x / factor, y / factor, z / factor };
	}

	constexpr const vector3& operator*(const float factor) const noexcept
	{
		return vector3{ x * factor, y * factor, z * factor };
	}

	// utils
	constexpr const vector3& ToAngle() const noexcept
	{
		return vector3{
			std::atan2(-z, std::hypot(x, y)) * (180.0f / std::numbers::pi_v<float>),
			std::atan2(y, x) * (180.0f / std::numbers::pi_v<float>),
			0.0f };
	}

	constexpr const bool IsZero() const noexcept
	{
		return x == 0.f && y == 0.f && z == 0.f;
	}

	// struct data
	float x, y, z;
};
