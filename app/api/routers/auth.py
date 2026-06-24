import logging
from datetime import timedelta
from fastapi import APIRouter, Depends, HTTPException, status
from fastapi.security import OAuth2PasswordRequestForm

from core.config import settings
from core.security import verify_password, create_access_token, get_current_user_id
from schemas.user import UserCreate, UserResponse, Token
from services.user_service import UserService, get_user_service

logger = logging.getLogger(__name__)

router = APIRouter(prefix="/auth", tags=["Authentication"])


@router.post("/register", response_model=UserResponse, status_code=status.HTTP_201_CREATED)
async def register(
    user_in: UserCreate,
    user_service: UserService = Depends(get_user_service)
):
    logger.info(f"Próba rejestracji nowego użytkownika: {user_in.email}")
    user = await user_service.get_user_by_email(user_in.email)
    if user:
        logger.warning(f"Odrzucono rejestrację: email '{user_in.email}' już istnieje")
        raise HTTPException(
            status_code=status.HTTP_400_BAD_REQUEST,
            detail="Użytkownik o tym adresie email już istnieje."
        )

    new_user = await user_service.create_user(user_in)
    logger.info(f"Zarejestrowano nowego użytkownika: {user_in.email} (id: {new_user['id']})")
    return new_user


@router.post("/login", response_model=Token)
async def login(
    form_data: OAuth2PasswordRequestForm = Depends(),
    user_service: UserService = Depends(get_user_service)
):
    logger.info(f"Próba logowania: {form_data.username}")
    user = await user_service.get_user_by_email(form_data.username)
    if not user or not verify_password(form_data.password, user["password_hash"]):
        logger.warning(f"Nieudane logowanie dla: {form_data.username}")
        raise HTTPException(
            status_code=status.HTTP_401_UNAUTHORIZED,
            detail="Niepoprawny email lub hasło.",
            headers={"WWW-Authenticate": "Bearer"},
        )

    access_token_expires = timedelta(minutes=settings.ACCESS_TOKEN_EXPIRE_MINUTES)
    access_token = create_access_token(
        subject=user["id"], expires_delta=access_token_expires
    )
    logger.info(f"Pomyślne logowanie: {form_data.username} (id: {user['id']})")
    return {"access_token": access_token, "token_type": "bearer"}


@router.get("/me", response_model=UserResponse)
async def read_users_me(
    current_user_id: int = Depends(get_current_user_id),
    user_service: UserService = Depends(get_user_service)
):
    logger.info(f"Pobieranie danych zalogowanego użytkownika: id {current_user_id}")
    user = await user_service.get_user_by_id(current_user_id)
    if not user:
        logger.warning(f"Nie znaleziono użytkownika o id {current_user_id} (/me)")
        raise HTTPException(status_code=404, detail="Użytkownik nie istnieje")
    logger.info(f"Zwrócono dane użytkownika o id {current_user_id}")
    return user
