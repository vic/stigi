// @ts-check
import { defineConfig, fontProviders } from 'astro/config';
import starlight from '@astrojs/starlight';
import catppuccin from '@catppuccin/starlight';

export default defineConfig({
	site: 'https://stigi.org',
	fonts: [
		{ provider: fontProviders.google(), name: 'Victor Mono', cssVariable: '--font-victor-mono' },
		{ provider: fontProviders.google(), name: 'JetBrains Mono', cssVariable: '--font-jetbrains-mono' },
	],
	integrations: [
		starlight({
			title: 'Stigi',
			description:
				'A systems language and verification door for the era when AI writes most of the code.',
			logo: { light: './src/assets/logo-light.svg', dark: './src/assets/logo.svg', alt: 'Stigi' },
			favicon: '/favicon.svg',
			social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/vic/stigi' }],
			editLink: { baseUrl: 'https://github.com/vic/stigi/edit/main/docs/' },
			components: {
				Head: './src/components/Head.astro',
				Header: './src/components/Header.astro',
				Hero: './src/components/Hero.astro',
				Sidebar: './src/components/Sidebar.astro',
				PageSidebar: './src/components/PageSidebar.astro',
				TableOfContents: './src/components/TableOfContents.astro',
				ThemeSelect: './src/components/ThemeSelect.astro',
				Footer: './src/components/Footer.astro',
			},
			plugins: [
				catppuccin({
					dark: { flavor: 'mocha', accent: 'mauve' },
					light: { flavor: 'latte', accent: 'mauve' },
				}),
			],
			customCss: ['./src/styles/layout.css', './src/styles/custom.css', './src/styles/stigi.css'],
			sidebar: [
				{ label: 'Start here', items: [{ autogenerate: { directory: 'start' } }] },
				{ label: 'The language', items: [{ autogenerate: { directory: 'language' } }] },
				{ label: 'The door', items: [{ autogenerate: { directory: 'door' } }] },
				{ label: 'Working with agents', items: [{ autogenerate: { directory: 'agents' } }] },
				{ label: 'Understanding code', items: [{ autogenerate: { directory: 'humans' } }] },
				{ label: 'The runtime envelope', items: [{ autogenerate: { directory: 'runtime' } }] },
				{ label: 'Platforms', items: [{ autogenerate: { directory: 'platforms' } }] },
				{ label: 'Trust', items: [{ autogenerate: { directory: 'trust' } }] },
				{ label: 'Related work', items: [{ autogenerate: { directory: 'related' } }] },
				{ label: 'Community', items: [{ autogenerate: { directory: 'community' } }] },
			],
		}),
	],
});
