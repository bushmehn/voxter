import { createParamDecorator, ExecutionContext } from '@nestjs/common';

export interface CurrentUserShape {
  userId: string;
  email: string;
}

export const CurrentUser = createParamDecorator(
  (_: unknown, ctx: ExecutionContext): CurrentUserShape => {
    const request = ctx.switchToHttp().getRequest();
    return request.user;
  },
);
