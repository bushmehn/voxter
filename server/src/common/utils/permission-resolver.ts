import { PermissionValue } from '../constants/permissions';

export interface PermissionOverwriteInput {
  allow: PermissionValue;
  deny: PermissionValue;
}

export function hasPermission(current: PermissionValue, required: PermissionValue): boolean {
  return (current & required) === required;
}

export function applyOverwrite(
  base: PermissionValue,
  overwrite?: PermissionOverwriteInput,
): PermissionValue {
  if (!overwrite) {
    return base;
  }
  const removed = base & ~overwrite.deny;
  return removed | overwrite.allow;
}

export function combinePermissions(permissions: PermissionValue[]): PermissionValue {
  return permissions.reduce((acc, value) => acc | value, 0n);
}
