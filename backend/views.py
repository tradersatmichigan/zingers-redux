from django.contrib.auth.decorators import login_required
from django.http import HttpRequest
from django.http.response import JsonResponse


@login_required
def get_user_info(request: HttpRequest) -> JsonResponse:
    return JsonResponse(
        {
            "user_id": request.user.id,
            "username": request.user.get_full_name() or request.user.username,
        },
        status=200,
    )
