FROM node:20-alpine AS development

WORKDIR /app

RUN npm install -g @expo/cli@latest @expo/ngrok@latest

COPY package*.json ./
RUN npm install

COPY . .

EXPOSE 8081
EXPOSE 19000
EXPOSE 19001
EXPOSE 19002

ENV EXPO_DEVTOOLS_LISTEN_ADDRESS=0.0.0.0

CMD ["npx", "expo", "start", "--host", "lan"]

FROM node:20-alpine AS production

WORKDIR /app

RUN npm install -g @expo/cli serve

COPY package*.json ./
RUN npm ci --only=production

COPY . .

RUN npx expo export -p web

EXPOSE 3000

CMD ["serve", "-s", "dist", "-l", "3000"]