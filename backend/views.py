from django.contrib.auth.decorators import login_required
from django.contrib.auth.models import User
from django.http import HttpRequest
from django.http.response import JsonResponse


@login_required
def get_user_info(request: HttpRequest) -> JsonResponse:
    user = User.objects.get(pk=request.user.pk)
    return JsonResponse(
        {
            "user_id": user.pk,
            "name": user.get_full_name(),
        },
        status=200,
    )
