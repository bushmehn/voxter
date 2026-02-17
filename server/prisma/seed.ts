import { PrismaClient } from '@prisma/client';
import { PrismaPg } from '@prisma/adapter-pg';
import * as argon2 from 'argon2';
import { PermissionBits, PermissionPreset } from '../src/common/constants/permissions';

const connectionString = process.env.DATABASE_URL;
if (!connectionString) {
  throw new Error('DATABASE_URL is required');
}

const prisma = new PrismaClient({
  adapter: new PrismaPg({ connectionString }),
});

async function main() {
  const adminEmail = 'admin@voxter.local';
  const existing = await prisma.user.findUnique({ where: { email: adminEmail } });
  if (existing) {
    return;
  }

  const user = await prisma.user.create({
    data: {
      email: adminEmail,
      passwordHash: await argon2.hash('ChangeMe123!'),
      displayName: 'Voxter Admin',
      presence: 'ONLINE',
    },
  });

  const guild = await prisma.guild.create({
    data: {
      name: 'Voxter Hub',
      ownerId: user.id,
    },
  });

  const everyoneRole = await prisma.role.create({
    data: {
      guildId: guild.id,
      name: '@everyone',
      isDefault: true,
      permissions: PermissionPreset.Member,
      position: 0,
    },
  });

  const adminRole = await prisma.role.create({
    data: {
      guildId: guild.id,
      name: 'Admin',
      permissions:
        PermissionBits.ViewChannel |
        PermissionBits.SendMessages |
        PermissionBits.ManageMessages |
        PermissionBits.ManageChannels |
        PermissionBits.ManageGuild |
        PermissionBits.Connect |
        PermissionBits.Speak |
        PermissionBits.MuteMembers,
      position: 1,
    },
  });

  await prisma.guildMember.create({
    data: {
      guildId: guild.id,
      userId: user.id,
      roleLinks: {
        create: [
          { roleId: everyoneRole.id },
          { roleId: adminRole.id },
        ],
      },
    },
  });

  await prisma.channel.createMany({
    data: [
      { guildId: guild.id, name: 'general', type: 'TEXT', position: 0 },
      { guildId: guild.id, name: 'voice', type: 'VOICE', position: 1 },
    ],
  });
}

main()
  .catch((error) => {
    // eslint-disable-next-line no-console
    console.error(error);
    process.exit(1);
  })
  .finally(async () => {
    await prisma.$disconnect();
  });
