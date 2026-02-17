export const PermissionBits = {
  ViewChannel: 1n << 0n,
  SendMessages: 1n << 1n,
  ManageMessages: 1n << 2n,
  ManageChannels: 1n << 3n,
  ManageGuild: 1n << 4n,
  Connect: 1n << 5n,
  Speak: 1n << 6n,
  MuteMembers: 1n << 7n,
} as const;

export const PermissionPreset = {
  Member:
    PermissionBits.ViewChannel |
    PermissionBits.SendMessages |
    PermissionBits.Connect |
    PermissionBits.Speak,
  Moderator:
    PermissionBits.ViewChannel |
    PermissionBits.SendMessages |
    PermissionBits.ManageMessages |
    PermissionBits.Connect |
    PermissionBits.Speak |
    PermissionBits.MuteMembers,
  Admin:
    PermissionBits.ViewChannel |
    PermissionBits.SendMessages |
    PermissionBits.ManageMessages |
    PermissionBits.ManageChannels |
    PermissionBits.ManageGuild |
    PermissionBits.Connect |
    PermissionBits.Speak |
    PermissionBits.MuteMembers,
} as const;

export type PermissionValue = bigint;
