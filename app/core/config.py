from pydantic_settings import BaseSettings, SettingsConfigDict

class Settings(BaseSettings):
    REDIS_URL: str = "rediss://localhost:6380/0"
    DATABASE_URL: str = "postgresql://postgres:postgres@localhost:5432/watering"
    API_KEY: str
    LOG_LEVEL: str = "INFO"
    JWT_SECRET_KEY: str = "supersecretkey12345" # In production, set this in .env
    ALGORITHM: str = "HS256"
    ACCESS_TOKEN_EXPIRE_MINUTES: int = 60 * 24 * 7 # 7 days
    
    model_config = SettingsConfigDict(env_file=".env", env_file_encoding="utf-8", extra="ignore")

settings = Settings()
