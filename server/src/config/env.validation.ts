export function validateEnv(config: Record<string, unknown>) {
  const required = [
    'DATABASE_URL',
    'REDIS_URL',
    'JWT_ACCESS_SECRET',
    'JWT_REFRESH_SECRET',
  ];

  for (const key of required) {
    if (!config[key]) {
      throw new Error(`Missing required env var: ${key}`);
    }
  }

  return config;
}
