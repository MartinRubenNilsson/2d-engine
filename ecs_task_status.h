#pragma once

namespace ecs {
	enum class TaskStatus {
		Preparing,
		Doing,
		Succeeded,
		Failed
	};
}