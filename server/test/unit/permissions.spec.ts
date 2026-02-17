import {
  applyOverwrite,
  combinePermissions,
  hasPermission,
} from '../../src/common/utils/permission-resolver';
import { PermissionBits, PermissionPreset } from '../../src/common/constants/permissions';

describe('permission-resolver', () => {
  it('combines multiple role permissions', () => {
    const combined = combinePermissions([PermissionBits.ViewChannel, PermissionBits.SendMessages]);
    expect(hasPermission(combined, PermissionBits.ViewChannel)).toBe(true);
    expect(hasPermission(combined, PermissionBits.SendMessages)).toBe(true);
    expect(hasPermission(combined, PermissionBits.ManageGuild)).toBe(false);
  });

  it('applies channel overwrite with deny then allow', () => {
    const base = PermissionPreset.Member;
    const overwritten = applyOverwrite(base, {
      allow: PermissionBits.ManageMessages,
      deny: PermissionBits.SendMessages,
    });

    expect(hasPermission(overwritten, PermissionBits.ManageMessages)).toBe(true);
    expect(hasPermission(overwritten, PermissionBits.SendMessages)).toBe(false);
  });
});
